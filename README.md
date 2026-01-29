# AMB82-mini Edge AI 智慧應用整合實作

## 📌 專案簡介
本專案展示了在 Realtek AMB82-mini 嵌入式平台上，如何結合硬體感測器與雲端生成式 AI (Gemini API) 實現多模態互動應用。

## 🛠 整合技術
- **硬體**: AMB82-mini (Ameba Pro 2), Camera, SD Card, Speaker (PAM8403), TFT LCD。
- **通訊**: WiFi 聯網、RESTful API 串接 (Gemini 2.0 Flash)。
- **語音處理**: Google Text-To-Speech (TTS) 整合。

## 💡 實作項目
1. **智慧監控系統**: 定時拍攝場景，由 Gemini 分析畫面內容。若場景文字描述發生顯著變化，則自動紀錄圖片與日誌至 SD 卡。
2. **盲人視覺輔助**: 透過觸摸感測器觸發，拍攝眼前物體或 QR Code，並將分析結果透過喇叭進行語音回饋。
3. **情緒感知播放器**: 擷取人臉畫面判定情緒，自動從 SD 卡挑選並播放對應氛圍的 MP3 音樂。

## 📑 設計思路
專案核心在於**異質功能模組化**。我將攝像頭驅動、WiFi 請求與語音播放器解耦，並利用 AI 輔助優化程式碼邏輯，最終在資源有限的 MCU 上實現穩定運作。
