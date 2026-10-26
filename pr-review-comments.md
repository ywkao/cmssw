# PR Review Comments — DQM/HGCAL

Reviewer: **gabrielmscampos**

> 註：截圖中被水平捲軸截斷的行以 `…` 標示，請以 repo 中的實際程式碼為準。

| # | 檔案 | 行數 | 主題 | 判斷 | 狀態 |
|---|------|------|------|------|------|
| 1 | `DQM/HGCAL/src/HGCalDQMGeometry.cc` | 167–178 | `SetBranchAddress` 指向已離開 scope 的 stack locals | ✅ 成立（實際 stack 寫入） | ✔ aadc2cc |
| 2 | `DQM/HGCAL/plugins/HGCalFastStreamDQM.cc` | 486–496 | static `timeRecorded` 跨 run 共用，改用 `run.beginTime()` | ✅ 成立 | ✔ ba73167 |
| 3 | `DQM/HGCAL/src/HGCalQualityWorker.cc` | — | `TH2Poly::AddBin` 取得 ownership，同一 `TGraph*` 被重複 delete | ✅ 成立（crash） | ✔ 4f5ebd9 |
| 4 | `DQM/HGCAL/src/HGCalChannelWorker.cc` | 580–583 | 以 `layer_idx` 索引導致 module 互相覆蓋 | ✅ 成立（已知 bug） | ✔ e8ec0c5 |
| 5 | `DQM/HGCAL/plugins/HGCalFastStreamDQM.cc` | 357–380 | `bookHistograms` 每個 run 執行但 containers 未清空 | ✅ 成立 | ✔ bda1306 |
| 6 | `DQM/HGCAL/plugins/HGCalFastStreamDQM.cc` | 211–213 | `setBinLabel` 與 nested map lookup 每 event 執行 | ✅ 成立（效能） | ✔ 7110220 |
| 7 | `DQM/HGCAL/src/HGCalQualityWorker.cc` | 173–182 | ME 缺失時 log error 後仍 dereference | ✅ 成立（需注意 index 對齊） | ✔ 1ee53a9 |

---

## 1. `DQM/HGCAL/src/HGCalDQMGeometry.cc` (L167–178)

### Selected code

```cpp
      double xmin, xmax, ymin, ymax;
      tree->SetBranchAddress("xmin", &xmin);
      tree->SetBranchAddress("xmax", &xmax);
      tree->SetBranchAddress("ymin", &ymin);
      tree->SetBranchAddress("ymax", &ymax);
      if (!(tree->GetEntries() > 0))
        edm::LogError("HGCalDQMGeometry") << "TTree empty in " << moduleDir;
      tree->GetEntry(0);
      box = BoundingBox(
          static_cast<float>(xmin), static_cast<float>(xmax), static_cast<float>(ymin), sta…
      gDirectory->cd("/");
      return box;
```

### Reviewer comment

> `SetBranchAddress` points the cached module_properties tree at stack locals that go out of scope.

### 討論 / 處理

**判斷：成立，比 reviewer 說的嚴重。** template file 被 cache，`module_properties` TTree 留在記憶體，branch address 指向上次呼叫的 stack。`build()` 對同一 module 先 `readModuleScope()`（xmin…）再 `readModuleCenter()`（x0/y0），後者的 `GetEntry(0)` 會把 xmin… 寫進已失效的 stack → 真的寫壞記憶體，不只是 dangling pointer。

**已修改（commit `aadc2cc`）：** 兩個 reader 在 `GetEntry(0)` 後 `tree->ResetBranchAddresses()`。

**觀察：** TTree 為空時只 log，`GetEntry(0)` 不會寫入 → 回傳未初始化的值；不在 reviewer 範圍，未改。

### Reply (English)

Good catch, thanks. It was worse than a dangling address: `build()` calls `readModuleScope()` and then `readModuleCenter()` on the same cached tree, so the second `GetEntry(0)` also wrote the `xmin/xmax/ymin/ymax` branches into the stale stack frame of the first call. Both readers now call `tree->ResetBranchAddresses()` right after `GetEntry(0)`.

Fixed in aadc2cc.

---

## 2. `DQM/HGCAL/plugins/HGCalFastStreamDQM.cc` (L486–496)

### Selected code

```cpp
  static std::atomic<bool> timeRecorded{false};
  if (!timeRecorded.exchange(true)) {
    auto now = std::chrono::system_clock::now();
    std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    double unixTimestamp = static_cast<double>(timestamp);
    runStartME->Fill(unixTimestamp);

    TDatime dt(timestamp);
    TString timeString = dt.AsSQLString();
    timeStampME = ibooker.bookString("timeStamp", timeString);
  }
```

### Reviewer comment

> sharing the time across runs on multi-run jobs wouldn't make DQM comparison non-reproducible? I mean, the first ever `bookHistograms` will book `timeStamp`, but others would keep 0 and no string ME.
>
> perhaps you could take the time from `run.beginTime()` instead.

### 討論 / 處理

**判斷：成立，接受 reviewer 建議。** process 層級的 `static std::atomic<bool>` 讓整個 job 只有第一個 stream、第一個 run 填值與 book `timeStamp`，其餘為 0；且用的是 booking 當下的 wall-clock，重跑不可重現。

**已修改（commit `ba73167`）：** `bookEventInfo(ibooker, run)` 每個 stream、每個 run 都用 `run.beginTime().unixTime()` 填 `runStartTimeStamp` 並 book `timeStamp`；`unixTime() == 0`（source 沒有 run time）時 fallback 回 `std::time(nullptr)`。移除 static atomic 與 `<atomic>`，補上 `<ctime>`、`Run.h`、`TDatime.h`。

**觀察：**
- wf 77（TB，`DAQSource`）的 `run.beginTime()` 是 `Timestamp::beginOfTime()`（value=1、`unixTime()=0`），**不是** `invalidTimestamp()`（0）→ 判斷條件用 `unixTime() == 0` 才能同時涵蓋兩者。
- 不 fallback 時 `TDatime(0)` 會印出 `2034-01-01`（`TDatime` 只支援 1995 年以後）。
- fallback 後 wf 77：`runStartTimeStamp=1793000549`、`timeStamp=2026-10-26 08:42:29`，與修改前行為一致（booking 當下時間）；有 run time 的 source 則可重現。

### Reply (English)

Agreed, thanks. `bookEventInfo` now takes the run and, in every stream and every run, fills `runStartTimeStamp` and books `timeStamp` from `run.beginTime().unixTime()`. The static flag is gone.

One caveat: the test-beam workflow (77, `DAQSource`) gives `beginOfTime()` as the run begin time (`unixTime() == 0`). In that case only, the code falls back to the booking time, which is what the code did before. Any source that provides a run time now gives reproducible output.

Fixed in ba73167.

---

## 3. `DQM/HGCAL/src/HGCalQualityWorker.cc`

### Selected code

```cpp
        for (const auto& cassettePair : cassetteMap)
          for (const auto& econdPair : cassettePair.second)
            for (const std::string& plotKey : hexPlotsFastStreamKey_)
              hexPlotsFastStream_[plotKey][layer]->addBin(geom.moduleBin(econdPair.second.dqmIndex));
```

### Reviewer comment

> `TH2Poly::AddBin` takes ownership of the polygon and deletes it in its destructor. Here, and in `ChannelWorker` / `TriggerWorker`, one `TGraph*` is added to several TH2Polys, so it is deleted several times when the MEs are destroyed.
>
> Perhaps add a separate copy per histogram.

### 討論 / 處理

**判斷：成立。** `TH2PolyBin` 的 destructor 會 `delete fPoly`，同一個 `TGraph*` 加進 N 個 TH2Poly 就會被 delete N 次。

**已修改（commit `4f5ebd9`）：** 每次 `addBin` 都改成 `addBin(new TGraph(*gr))`，影響範圍：
- QualityWorker：layer hex 圖，使用 geometry outline
- ChannelWorker：module、cassette、layer 三層圖
- TriggerWorker：module、cassette、layer 三層圖

連帶修改：
- `ReadObj()` 的結果改用 `std::unique_ptr` 持有。被 `continue` 跳過的物件和非 TGraph 物件不再 leak（文末 B）。
- 原本用 `new` 建立的中繼 graph（`grLayer`、`TCBinCassette`）改成 stack 物件。
- `moduleBin()` / `triggerModuleBin()` 改回傳 `TGraph const*`，header 的 ownership 註解也一併更正（文末 C）。
- ChannelWorker 以前會 `SetName` 共用的 outline，現在改成在本地 copy 上設定。
- outline 為 nullptr 的處理：
  - QualityWorker、TriggerWorker：改成 throw。若只是跳過，會造成後面 bin 錯位。
  - ChannelWorker：沿用原本的 LogError 加跳過。

**觀察：**
- geometry 的 outline TGraph 不屬於 TFile 管理（TGraph 不會註冊到 directory），目前到 job 結束都沒有釋放。這只是一次性的小量 leak，不在這個 PR 處理。
- wf 77 修改前後 hex 圖 bin 數與內容完全相同（copy 不影響顯示）。
- 修改前的 code 用 `cmsRun` 和 `cmsRunGlibC` 都**沒有 crash**：`DQMStore` 以 raw pointer 持有 ME、destructor 是空的，job 結束時 ME 不會被 delete → 單 run job 中這個 bug 是潛伏的。會真的 double free 的時機是 `DQMStore::cleanupLumi` delete ME（multi-run、online 長時間跑的 run 切換）。回覆 reviewer 時可說明。

### Reply (English)

Right, thanks. Every `addBin` now passes its own copy (`new TGraph(*gr)`), in `HGCalQualityWorker`, `HGCalChannelWorker` and `HGCalTriggerWorker`. The objects returned by `ReadObj()` are now held in a `std::unique_ptr`, which also fixes a leak of the skipped/non-`TGraph` keys. The geometry outlines (`moduleBin()` / `triggerModuleBin()`) are now returned as `const`, so they can't be handed to a `TH2Poly` again. The ownership comment in the header is corrected too.

For context, we could not reproduce a crash in a single-run job, even with `cmsRunGlibC`, because the `DQMStore` does not delete MEs at the end of the job. It would hit once MEs are deleted on run transitions. The output of workflow 77 is unchanged by this fix.

Fixed in 4f5ebd9.

---

## 4. `DQM/HGCAL/src/HGCalChannelWorker.cc` (L580–583)

### Selected code

```cpp

            // Per-FED module summary.
            uint32_t const fedid = ele.fedid;
            int const fedBin = fed_module_bin[fedid][layer_idx] + 1;
```

### Reviewer comment

> As the comment at L256 says, this indexes by layer_idx, so modules overwrite each other in `summaryPerModule_FED*`.

### 討論 / 處理

**判斷：成立（L256 註解所記的已知 bug）。** `fed_module_bin` 以 global module 序號存、卻以 `layer_idx` 查 → 同 layer 的 module 落在同一個 bin 互相覆蓋。

**已修改（commit `e8ec0c5`）：** 刪掉 `fed_module_bin` / `globalModuleIndex` / Known issue 註解；第二個迴圈用 per-FED counter `fedBin = ++fed_bin_counter[fedid]`，與 X 軸 typecode label 同為 `HGCALMap` 順序。counter 放在 module 迴圈開頭（缺 ME / template 的 `continue` 之前），被跳過的 module 也占一個 bin，後面不會錯位。

### Reply (English)

Thanks, fixed. `fed_module_bin` (and the known-issue comment) is gone. The bin is now a per-FED counter that follows the same `HGCALMap` order as the x-axis typecode labels. It is incremented before any module is skipped, so the following bins stay aligned. In workflow 77, `summaryPerModule_FED2001` now has one bin per module.

Fixed in e8ec0c5.

---

## 5. `DQM/HGCAL/plugins/HGCalFastStreamDQM.cc` (L357–380)

### Selected code

```cpp
    if (fed.totalECONs_ == 0)
      continue;
    followedFEDs_.push_back(fed.id);
    fedIdToBinMap_[fed.id] = binIndex;
    binIndex++;
  }

  // Loop over available modules and select those to track
  for (auto ele : hgcal::dqm::readoutModules(moduleIndexer, moduleInfo)) {
    const uint32_t fedid = ele.fedid;
    const uint32_t imod = ele.modid;
    const std::string& typecode = ele.typecode;
    unique_directionallayers.insert(ele.zside ? static_cast<int>(ele.layer) : -static_cast…

    // Store in maps
    MonitoredElementKey_t key(fedid, imod);
    ele.fedModuleIndex = modulesByFED_[fedid].size();
    modulesByFED_[fedid].emplace_back(key, ele);
    followedModules_[key] = ele;

    // Build hierarchical HGCALMap: endcap -> layer -> cassette -> typecode
    auto& cassetteMap = HGCALMap[ele.endcap][ele.layer][ele.cassette];
    cassetteMap[typecode] = ele;
    cassetteMap[typecode].moduleIndex = cassetteMap.size() - 1;
```

### Reviewer comment

> `bookHistograms` runs once per run, but these containers are never cleared. On a second run followedFEDs_/modulesByFED_ double, and moduleIndex = size()-1 assigns the same index to every existing module.
>
> same pattern in `HGCalLayerClusterDQM.cc:184-197` and `HGCalRecHitDQM.cc:201-213`.
>
> This could be a problem in reprocessing jobs, where DQMIO files can have more than one run per file.

### 討論 / 處理

**判斷：成立。** 第二個 run 起：FED 數翻倍（bin 數錯）、`fedModuleIndex` 超出 bin、`moduleIndex` 全部相同。

**已修改（commit `bda1306`）：**
| Plugin | 處理 |
|---|---|
| `HGCalFastStreamDQM` | 開頭 `clear()`：`followedFEDs_`、`fedIdToBinMap_`、`modulesByFED_`、`followedModules_`、`HGCALMap`、`unique_directionallayers` |
| `HGCalLayerClusterDQM` | 刪掉只寫不讀的 `followedModules_`、`modulesByFED_`；`HGCALMap.clear()` |
| `HGCalRecHitDQM` | 刪掉只寫不讀的 `modulesByFED_`；`followedModules_`、`HGCALMap` 先 clear |
| `HGCalDigiDQM` | map overwrite，等冪，不需改 |

LayerCluster / RecHit 的 `moduleIndex` 沒人讀，一併拿掉計算。

**觀察：** ME pointer map（`econdQualityCassettes_`、`econdQualityFEDs_`、`fedComparisonHistos_`、`moduleHistos_`）不清：rebook 會覆蓋現有 key，analyze 只用目前 run 的 key 查，殘留的 stale key 不會被讀到。harvester 端同類問題見文末 A。

### Reply (English)

Thanks, good point for reprocessing. `HGCalFastStreamDQM::bookHistograms` now clears `followedFEDs_`, `fedIdToBinMap_`, `modulesByFED_`, `followedModules_`, `HGCALMap` and `unique_directionallayers` before rebuilding them. In `HGCalLayerClusterDQM` and `HGCalRecHitDQM`, most of these containers were written but never read, so I removed them (`followedModules_` and `modulesByFED_` in the former, `modulesByFED_` in the latter). The ones still used are cleared in the same way. The unused `moduleIndex` computation is dropped too.

While checking this, I found the same multi-run pattern on the harvester side: `HGCalChannelWorker::endRun` and `HGCalTriggerWorker` add the `TH2Poly` bins on every `endRun`, but the MEs are booked only once. I'd like to address that in a follow-up PR, together with a proper multi-run test input, if that's OK with you.

Fixed in bda1306.

---

## 6. `DQM/HGCAL/plugins/HGCalFastStreamDQM.cc` (L211–213)

### Selected code

```cpp
    econdQualityH_ = econdQualityCassettes_[endcap][layer][cassette];
    econdPayload_ = econdPayloadCassettes_[endcap][layer][cassette];
    econdQualityH_->setBinLabel(bin_id + 1, binlabel, 1);
```

### Reviewer comment

> `setBinLabel` (here and at L297) plus the nested map lookups run per module, per event. Could the labels be set once at booking, and the ME pointers and bin index cached per module?

### 討論 / 處理

**判斷：成立（效能）。** 每 event × 每 module：兩次 `setBinLabel`（L213、L297）、4 層 `HGCALMap` + 兩個 3 層 ME map、每個 eRx 兩次 `moduleHistos_["cmChannels"][key]`、typecode string copy。

**已修改（commit `7110220`）：**
- **label 移到 booking**：cassette ME 在 `bookHistogramsFastStream`、per-FED ME 在 `bookPerFEDComparisonHistograms` 各設一次。
- **`moduleIndex` 先算再 copy**：原本 `followedModules_[key] = ele` 在 `moduleIndex` 算出前就 copy，analyze 才要回查 `HGCALMap`；現在 `ele.moduleIndex = cassetteMap.size()` 先設好（值與原本 `size()-1` 相同）。
- **快取 ME**：新增 `std::vector<ModuleMEs> moduleMEs_`（module + quality/payload/cm 三個 ME），取代 `moduleHistos_`；`analyzeECONDFlags` 直接走 vector。per-FED ME 在 FED 迴圈外取一次。
- 刪掉暫存用 members `econdQualityH_`、`econdPayload_`。

**觀察：** label 從 booking 就存在，harvester `findBinByLabel`（#7）不再依賴「至少跑過一個 event」。

### Reply (English)

Done, thanks. The cassette and per-FED ECON-D bin labels are now set once at booking. `moduleIndex` is assigned before the module is copied into `followedModules_` / `modulesByFED_`. Each module's quality/payload/CM MEs are cached in a vector at booking, so `analyze` no longer calls `setBinLabel` or does map lookups per module. As a side effect, the labels exist even if no event was processed, which the harvester's label lookup relies on.

Fixed in 7110220.

---

## 7. `DQM/HGCAL/src/HGCalQualityWorker.cc` (L173–182)

### Selected code

```cpp
            if (!quality_me) {
              edm::LogError("HGCalQualityWorker") << "Did not find quality_me in " << cass…
            }
            if (!payload_me) {
              edm::LogError("HGCalQualityWorker") << "Did not find payload_me in " << cass…
            }

            // fill layer-level ECON-D TH2F
            econd_error_summarizer_.processAndFill(quality_me, econdQuality_, cassette_idx…
            econdPayload_->getTH1()->Add(payload_me->getTH1());
```

### Reviewer comment

> If either ME is missing you log an error and then dereference it on the next lines, that is, `processAndFill` tries to `payload_me->getTH1()` and `quality_me->getNbinsY()`. Perhaps you need to `conitnue` inside the conditionals?

### 討論 / 處理

**判斷：成立。**（reviewer 原文的 `conitnue` 應為 `continue`）缺 ME 時 `processAndFill` 與後續 `getTH1()` / `getNbinsY()` 會 dereference nullptr。

**已修改（commit `1ee53a9`）：** 兩個 check 合併，缺任一個就 log 並 `continue`；`continue` 前把 `module_bin_index += 該 cassette 的 module 數`、`cassette_idx++`，否則後面所有 cassette 的 bin 會錯位（reviewer 沒提到這點）。

**觀察：** `findBinByLabel` 找不到回傳 -1 → 讀到 underflow（0），不會 crash，不處理。

### Reply (English)

Thanks, fixed. A missing cassette ME now logs an error and skips that cassette. Before the `continue`, it also advances `cassette_idx` and the `TH2Poly` module bin index by the cassette's module count; with a plain `continue`, every following cassette would have been filled into the wrong bins.

Fixed in 1ee53a9.

---

## 額外發現（reviewer 沒提）

### A. Harvester 在每個 run 的 endRun 重複 `addBin`（multi-run）— ➜ 另開 follow-up PR
- `HGCalChannelWorker::endRun`、`HGCalTriggerWorker::fillHexaPlots` 對只 book 一次的 TH2Poly 每個 run 都 `addBin` → 第二個 run 起 bin 數倍增，`setBinContent(i)` 只填前段。與 #5 同屬「一個檔多個 run」。
- 傾向做法：bin 只在第一次加，之後只 `setBinContent`（QualityWorker 已是此模式：`book()` 加 bin、endLumi 只填值）。
- 同類：ChannelWorker 的 overflow `TPolyMarker` 每個 run 都再加進 `GetListOfFunctions()`，也要先清。
- 風險：bin 佈局由第一個 run 決定（第一個 run 缺 ME 的 module 之後才出現會錯位）。
- 決定（2026-10-26）：不在此 PR 修；follow-up PR 一併準備 multi-run 測試輸入。在 #5 的回覆中告知 reviewer。

### B. `ReadObj()` 物件 leak — ✔ 隨 #3 修掉（`4f5ebd9`）
### C. Geometry ownership 註解錯誤 — ✔ 隨 #3 修掉（`4f5ebd9`）

---

## 進度與測試

Commits（依修改順序，皆未 push）：

| 順序 | Comment | Commit |
|---|---|---|
| 1 | #3 TH2Poly ownership（+B、C） | `4f5ebd9` |
| 2 | #7 null ME `continue` | `1ee53a9` |
| 3 | #1 `ResetBranchAddresses` | `aadc2cc` |
| 4 | #4 per-FED bin | `e8ec0c5` |
| 5 | #5 per-run container clear | `bda1306` |
| 6 | #6 label 移到 booking / ME 快取 | `7110220` |
| 7 | #2 `run.beginTime()` + fallback | `ba73167`（amend） |

每個 commit 都已 clang-format、`scram b` 無 error / warning。

測試（2026-10-25）：
- [x] `testHGCalDQMClients_cfg.py`：exit 0（#2 fallback 後重跑亦同）
- [x] `runTheMatrix.py -l 77 --ibeos`（TB reco+DQM → harvesting）：修改前、後都 PASSED，log 無 `%MSG-e`
  - 環境：本 IB 的 cms-data 沒有 `Geometry/HGCalMapping/data/DQM`，從 `CMSSW_20_1_X_2026-10-13-2300/external/.../data` 複製到本 area 的 `external/el9_amd64_gcc14/data/` 才能跑 harvesting
- [x] 修改前、後的 DQM 檔逐一比對 379 個 ME（bin 數、內容、X 軸 label）：
  - `summaryPerModule_FED2001` 不同 → 符合 #4 預期（修改前同一 layer 的 module 擠在同一 bin）
  - `runStartTimeStamp` / `timeStamp` 不同 → 預期（booking 當下時間；#2 fallback 後重測 2026-10-26 仍 PASSED）
  - 其餘全部相同（hex 圖、`econdQuality*` label 等）
- [ ] multi-run 輸入（#5、A）：目前沒有現成的多 run 輸入 → 隨 A 的 follow-up PR 準備
