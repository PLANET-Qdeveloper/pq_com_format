//! スペースバルーン通信フォーマット．
//!
//! [`format`] がフレームのエンコード・デコードを行い，
//! [`downlink`] がペイロード(TLV)を対応表に従って解釈する．

pub mod downlink;
pub mod format;
