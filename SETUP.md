# Source files

1. Copy `secrets.example.h` and rename the copy to `secrets.h`.
2. Enter your Wi-Fi and Blynk credentials in `secrets.h`.
3. Keep `secrets.h` private. It is already listed in `.gitignore`.
4. Open `PlantWaterer.ino` in Arduino IDE.
5. Set each Blynk moisture datastream to a range of **0 to 100**.

## Calibration

Update these values in `PlantWaterer.ino` using measurements from your own sensors:

```cpp
constexpr int RAW_WET = 1200;
constexpr int RAW_DRY = 3200;
```

The values remain raw ESP32 ADC readings internally. The dashboard receives a converted moisture percentage:

- `0%` = dry
- `100%` = wet
