#define I2S_DOUT I2S_DIN
#define I2S_LRC I2S_LRCLK
#define SD_PIN I2S_SD_MODE // Shutdown
//--------------------------------------------------------------------------------------
#include "Arduino.h"
#include "Audio.h"
#include "WiFi.h"
// #include "SD.h"
#include "FS.h"

// Digital I/O used
// #define SD_CS 5
// #define SPI_MOSI 6
// #define SPI_MISO 7
// #define SPI_SCK 8

Audio audio;

String ssid = "Leap";
String password = "awsedrft";

void setup() {
  // pinMode(SD_CS, OUTPUT);
  // digitalWrite(SD_CS, HIGH);
  Serial.begin(115200);

  pinMode(SD_PIN, OUTPUT);
  digitalWrite(SD_PIN, HIGH);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED)
    delay(1500);
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  // audio.setBalance(0);  // mutes the left channel
  audio.setVolume(21); // default 0...21
                       //  or alternative
                       //  audio.setVolumeSteps(64); // max 255
                       //  audio.setVolume(63);
                       //
                       //  *** radio streams ***
  // audio.connecttohost("http://stream.antennethueringen.de/live/aac-64/stream.antennethueringen.de/");
  // // aac
  // audio.connecttohost("http://mcrscast.mcr.iol.pt/cidadefm"); // mp3
  //  audio.connecttohost("http://www.wdr.de/wdrlive/media/einslive.m3u"); //
  //  m3u audio.connecttohost("https://stream.srg-ssr.ch/rsp/aacp_48.asx"); //
  //  asx audio.connecttohost("http://tuner.classical102.com/listen.pls"); //
  //  pls
  // audio.connecttohost("http://stream.radioparadise.com/flac"); // flac
  /// audio.connecttohost("http://stream.sing-sing-bis.org:8000/singsingFlac");
  /// // flac (ogg)
  //  audio.connecttohost("http://s1.knixx.fm:5347/dein_webradio_vbr.opus"); //
  //  opus (ogg)
  //  audio.connecttohost("http://stream2.dancewave.online:8080/dance.ogg"); //
  //  vorbis (ogg)
  //  audio.connecttohost("http://26373.live.streamtheworld.com:3690/XHQQ_FMAAC/HLSTS/playlist.m3u8");
  //  // HLS
  //  audio.connecttohost("http://eldoradolive02.akamaized.net/hls/live/2043453/eldorado/master.m3u8");
  //  // HLS (ts)
  //  *** web files ***
  //  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Pink-Panther.wav");
  //  // wav
  audio.connecttohost(
      "https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/"
      "additional_info/Testfiles/Santiano-Wellerman.flac");
  //  // flac
  //  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Olsen-Banden.mp3");
  //  // mp3
  //  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Miss-Marple.m4a");
  //  // m4a (aac)
  //  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Collide.ogg");
  //  // vorbis
  //  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/sample.opus");
  //  // opus
  //  *** local files ***
  //  audio.connecttoFS(SD, "/test.wav");     // SD
  //  audio.connecttoFS(SD_MMC, "/test.wav"); // SD_MMC
  //  audio.connecttoFS(SPIFFS, "/test.wav"); // SPIFFS

  //  audio.connecttospeech("Wenn die Hunde schlafen, kann der Wolf gut Schafe
  //  stehlen.", "de"); // Google TTS

  //  audio.connecttohost("http://vis.media-ice.musicradio.com/CapitalMP3");
}

void loop() { audio.loop(); }

// optional
void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
}
void audio_id3data(const char *info) { // id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}
void audio_eof_mp3(const char *info) { // end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
}
void audio_showstation(const char *info) {
  Serial.print("station     ");
  Serial.println(info);
}
void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle ");
  Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     ");
  Serial.println(info);
}
void audio_commercial(const char *info) { // duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info) { // homepage
  Serial.print("icyurl      ");
  Serial.println(info);
}
void audio_lasthost(const char *info) { // stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
void audio_eof_speech(const char *info) {
  Serial.print("eof_speech  ");
  Serial.println(info);
}
