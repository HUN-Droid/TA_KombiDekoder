static LedColor getLedColor(JelzoMode mode, uint8_t bit) {
    uint8_t lb; // local bit within group
    switch (mode) {
        case JelzoMode::UNI_4x2:
            return (bit % 2 == 0) ? LedColor::RED : LedColor::GREEN;

        case JelzoMode::UNI_2x3_1x2:
            if (bit < 6) {
                lb = bit % 3;
                if (lb == 0) return LedColor::RED;
                if (lb == 1) return LedColor::GREEN;
                return LedColor::YELLOW;
            }
            return (bit == 6) ? LedColor::RED : LedColor::GREEN;

        case JelzoMode::HV_2x4_elo:
            lb = bit % 4;
            return (lb < 2) ? LedColor::YELLOW : LedColor::GREEN;

        case JelzoMode::HV_1x5_1x3: {
            static const LedColor c5[] = {LedColor::RED, LedColor::RED, LedColor::GREEN, LedColor::YELLOW, LedColor::WHITE};
            static const LedColor c3[] = {LedColor::RED, LedColor::GREEN, LedColor::YELLOW};
            return (bit < 5) ? c5[bit] : c3[bit - 5];
        }

        case JelzoMode::MAV_2x4:
            lb = bit % 4;
            if (lb == 0) return LedColor::RED;
            if (lb == 1) return LedColor::GREEN;
            return LedColor::YELLOW;

        case JelzoMode::MAV_1x5_1x3:
        case JelzoMode::SK_1x5_1x3: {
            static const LedColor c5m[] = {LedColor::RED, LedColor::GREEN, LedColor::YELLOW, LedColor::YELLOW, LedColor::WHITE};
            static const LedColor c3m[] = {LedColor::RED, LedColor::GREEN, LedColor::YELLOW};
            return (bit < 5) ? c5m[bit] : c3m[bit - 5];
        }

        case JelzoMode::MAV_Fenysorompo_1x2:
            if (bit < 6) return (bit == 2 || bit == 5) ? LedColor::WHITE : LedColor::RED;
            return (bit == 6) ? LedColor::RED : LedColor::GREEN;

        case JelzoMode::SK_2x4:
            lb = bit % 4;
            if (lb == 0) return LedColor::RED;
            if (lb == 1) return LedColor::GREEN;
            if (lb == 2) return LedColor::YELLOW;
            return LedColor::WHITE;

        default:
            return LedColor::WHITE;
    }
}

class WSJelzoController : public IJelzoController {
public:
    WSJelzoController() :
        leds(&hspi1, spiBusyCheck, wsSpiStart, spiDone, GPIOB, GPIO_PIN_12) {
        }

    void init() override {
        leds.init(0,WS_JELZO_PORT_COUNT*8,0);
    }

    void write(uint8_t blinkPhase) override {
        leds.write(blinkPhase);
    }

    void onDMADone() override {
        leds.onDMADone();
    }

    /*void updateDisplay(uint8_t portIndex) override {
        // Végigmegyünk a port ledjein
        for (int bit = 0; bit < 8; bit++) {
            // Globális bit index számolása (0–23 között)
            int sourceIndex = portIndex * 8 + bit;

            // jelzőfej meghatározása bit alapján
            int jelzoGroup = 0;

            switch( jelzok[portIndex].mode ) {
                case JelzoMode::UNI_4x2 : // 4 db 2 lencsés
                    jelzoGroup = bit/2;
                break;
                case JelzoMode::UNI_2x3_1x2: // 2 db 3 lencsés + 1 db 2 lencsés
                    if (bit < 3) jelzoGroup = 0;
                    else if (bit < 6) jelzoGroup = 1;
                    else jelzoGroup = 2;
                break;
                case JelzoMode::HV_2x4_elo: // 2 db 4 lencsés német rendszerű
                case JelzoMode::MAV_2x4: // 2 db 4 lencsés magyar rendszerű
                    jelzoGroup = bit / 4;
                break;
                case JelzoMode::HV_1x5_1x3: // 1 db 5 lencsés + 1 db 3 lencsés német rendszerű
                case JelzoMode::MAV_1x5_1x3: // 1 db 5 lencsés + 1 db 3 lencsés magyar rendszerű
                    if (bit < 5) jelzoGroup = 0;
                    else jelzoGroup = 1;
                break;
                case JelzoMode::MAV_Fenysorompo_1x2:
                    if (bit < 6) jelzoGroup = 0;
                    else jelzoGroup = 1;
                break;
            }    
            
            // Át kell váltani kijelző szerinti ledIndex-re
            int group = sourceIndex / 3;                   // LED csoport (melyik LED)
            int bgr_bit = sourceIndex % 3;                 // BGR sorrendű bit a portData szerint
            int rgb_bit = 2 - bgr_bit;                     // átszámítás kijelző RGB sorrendbe
            int displayIndex = group * 3 + rgb_bit;        // RGB sorrendű LED index

            // eredeti, fade nélkül
            //setPixel(displayIndex, state);

            // egyszerű fade
            bool newState = (jelzok[portIndex].portData >> bit) & 1;
            bool currentState = leds.getBrightness(displayIndex) > 0;
            uint8_t speed = jelzok[portIndex].fadeSpeed[jelzoGroup];

            // villogtatás
            if(newState) { // csak a bekapcsoltnál van értelme
                //if(jelzok[portIndex].villog & (1<<bit) ) newState = false;
                if(blinkPhase && !(jelzok[portIndex].blinkPhaseOn & 0x01<<bit)) newState = false;
                if(!blinkPhase && !(jelzok[portIndex].blinkPhaseOff & 0x01<<bit)) newState = false;
            }

            // beállítás
            if (currentState && !newState) {
                if(speed==0) {
                    leds.setBrightness(displayIndex, 0);
                } else {
                    // Fade out            
                    leds.startFade(displayIndex, 0, speed); // Sebesség például 5
                }
            } else if (!currentState && newState) {
                if(speed==0) {
                    leds.setBrightness(displayIndex, jelzok[portIndex].brightness[jelzoGroup]);
                } else {
                    // Fade in
                    leds.startFade(displayIndex, jelzok[portIndex].brightness[jelzoGroup], jelzok[portIndex].fadeSpeed[jelzoGroup]); // Maximum fényerővel fade-in, sebesség 5
                }
                leds.set(displayIndex, WS2811_ON);
            }
            // Ha currentState == newState, nem csinálunk semmit!       

        }
    }*/
void updateDisplay(uint8_t portIndex) override {

    for (int bit = 0; bit < 8; bit++) {

        int sourceIndex = portIndex * 8 + bit;

        // --- jelző csoport meghatározás ---
        int jelzoGroup = 0;

        switch (jelzok[portIndex].mode) {
            case JelzoMode::UNI_4x2:
                jelzoGroup = bit / 2;
                break;

            case JelzoMode::UNI_2x3_1x2:
                if (bit < 3) jelzoGroup = 0;
                else if (bit < 6) jelzoGroup = 1;
                else jelzoGroup = 2;
                break;

            case JelzoMode::HV_2x4_elo:
            case JelzoMode::MAV_2x4:
            case JelzoMode::SK_2x4:
                jelzoGroup = bit / 4;
                break;

            case JelzoMode::HV_1x5_1x3:
            case JelzoMode::MAV_1x5_1x3:
            case JelzoMode::SK_1x5_1x3:
                jelzoGroup = (bit < 5) ? 0 : 1;
                break;

            case JelzoMode::MAV_Fenysorompo_1x2:
                jelzoGroup = (bit < 6) ? 0 : 1;
                break;
        }

        // --- LED index átszámítás ---
        int group = sourceIndex / 3;
        int bgr_bit = sourceIndex % 3;
        int rgb_bit = 2 - bgr_bit;
        int displayIndex = group * 3 + rgb_bit;

        // --- alap állapot ---
        bool baseState = (jelzok[portIndex].portData >> bit) & 1;

        // --- villogás alkalmazása ---
        bool finalState = baseState;

        if (baseState) {
            if (blinkPhase) {
                if (jelzok[portIndex].blinkPhaseOn & (1 << bit)) {
                    finalState = false;
                }
            } else {
                if (jelzok[portIndex].blinkPhaseOff & (1 << bit)) {
                    finalState = false;
                }
            }
        }

        // --- cél fényerő ---
        uint8_t targetBrightness = 0;
        if (finalState) {
            LedColor color = getLedColor(jelzok[portIndex].mode, bit);
            uint8_t dsIdx = jelzok[portIndex].datasetIndex;
            if (dsIdx >= BRIGHTNESS_DATASET_COUNT) dsIdx = 0;
            targetBrightness = brightnessDatasets[dsIdx].level[activeBrightnessLevel].brightness[static_cast<uint8_t>(color)];
        }

        uint8_t speed = jelzok[portIndex].fadeSpeed[jelzoGroup];

        // --- fade indítása CSAK ha változott a cél ---
        if (leds.getFadeTarget(displayIndex) != targetBrightness) {
            leds.startFade(displayIndex, targetBrightness, speed);
        }
    }
}   

    /**
     * Jelző loop - periódikusan meghívandó
     * ledek fade effektje
     * villogtatási logika
     */
    void loop() override {
        if (millis() - lastBlinkUpdate > 500) {
            lastBlinkUpdate = millis();
            blinkPhase = !blinkPhase;

            for(int i=0;i<getJelzokCount();i++) {
                updateDisplay(i);
            }
        }

        if (millis() - lastFadeUpdate > 20) { // kb 50 fps
            lastFadeUpdate = millis();
            leds.updateFades();
        }

        if(leds.isNeedUpdate()) {
            leds.write(1);
        }    
    }

    jelzo_t& getJelzo(uint8_t index) override {
        return jelzok[index];
    }

    uint8_t getJelzokCount() override {
        return WS_JELZO_PORT_COUNT;
    }

private:
    WS2811 leds;
    uint32_t lastFadeUpdate = 0;
    uint32_t lastBlinkUpdate = 0;
    bool blinkPhase = 1;

    jelzo_t jelzok[WS_JELZO_PORT_COUNT];
};
