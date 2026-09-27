# WLED-sync
Library to create WLED compatible projects that sync their audio

Got an existing WLED 0.14 with AudioRective UserMod or WLED-SR setup, but want to pass the audio data to your own projects?
Have an existing project with FFT audio analysis that you want to add WLED devices to?

Then this is the library for you, see two examples

If you do use this libray in your project, I would love to hear from you, please email will - at - netmindz.net

## ESPHome

Prefer ESPHome over the Arduino library? `esphome/components/wled_sync/` is an
ESPHome external component that receives the same UDP sound-sync data and
exposes it as `sensor`/`binary_sensor` entities (volume, FFT magnitude/major
peak, beat detection). See `esphome/test/wled-sync-receiver.yaml` for a
working example config. Currently targets ESP32 with the ESP-IDF framework.

## See Also

Stream WLED audio data from your PC with https://github.com/Victoare/SR-WLED-audio-server-win

Use WLED audio data with [Chataigne](http://benjamin.kuperberg.fr/chataigne/en#home) (Artist-friendly Modular Machine for Art and Technology) using https://github.com/zak-45/WLEDAudioSync-Chataigne-Module
