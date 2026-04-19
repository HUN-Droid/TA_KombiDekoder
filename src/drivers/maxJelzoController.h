class MAXJelzoController : public IJelzoController {
public:
    MAXJelzoController() :
        max7219(&hspi2, spiBusyCheck, maxSpiStart, spiDone, GPIOB, GPIO_PIN_3, MAX_CHIP_COUNT) {}

    void init() override {
        max7219.init();
    }

    void write(uint8_t blinkPhase) override {
        max7219.sendAllRows();
    }

    void onDMADone() override {
        max7219.onDMADone();
    }

    void updateDisplay(uint8_t portIndex) override {
        max7219.setRow(portIndex/8, portIndex%8, jelzok[portIndex].portData);
    }

    void loop() override {
        /*if (millis() - lastFadeUpdate > 500) { 
            lastFadeUpdate = millis();
            step++;
            if(step==8) step=0;
            max7219.setRow(0,2,1<<step);
            //max7219.sendAllRows();
        }

        if(max7219.isNeedUpdate()) {
            max7219.sendAllRows();
        }*/
    }

    jelzo_t& getJelzo(uint8_t index) override {
        return jelzok[index];
    }

    uint8_t getJelzokCount() override {
        return MAX_CHIP_COUNT * 8;
    }

private:
    MAX7219_Controller max7219;
    jelzo_t jelzok[MAX_CHIP_COUNT * 8];
    uint32_t lastFadeUpdate = 0;
    uint8_t step = 0;
};
