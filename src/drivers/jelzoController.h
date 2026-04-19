enum class JelzoMode : uint8_t {
    Bit,
    UNI_4x2, // 1 - 4 db 2 lencsés
    UNI_2x3_1x2, // 2 - 2 db 3 lencsés + 1 db 2 lencsés
    HV_2x4_elo, // 3 - 2 db 4 lencsés német rendszerű előjelző
    HV_1x5_1x3, // 4 - 1 db 5 lencsés + 1 db 3 lencsés német rendszerű
    MAV_2x4, // 5 - 2 db 4 lencsés magyar rendszerű
    MAV_1x5_1x3, // 6 - 1 db 5 lencsés + 1 db 3 lencsés magyar rendszerű
    MAV_Fenysorompo_1x2, // 7 - 2 db 3 lencsés fénysorompó + 1 db 2 lencsés
    SK_2x4, // 8 - 2 db 4 lencsés szlovák rendszerű
    SK_1x5_1x3 // 9 - 1 db 5 lencsés + 1 db 3 lencsés szlovák rendszerű
};

typedef struct {
    JelzoMode mode;
    uint8_t portData;
    uint8_t alapallas[4];
    uint8_t brightness[4];
    uint8_t fadeSpeed[4];
    uint8_t blinkPhaseOn;
    uint8_t blinkPhaseOff;
} jelzo_t;

class IJelzoController {
public:
    virtual void init() = 0;
    virtual void write(uint8_t blinkPhase) = 0; 
    virtual void onDMADone() = 0; 
    virtual void updateDisplay(uint8_t portIndex) = 0;
    virtual jelzo_t& getJelzo(uint8_t index) = 0;
    virtual uint8_t getJelzokCount() = 0;
    virtual void loop() = 0;
    virtual ~IJelzoController() = default;
};

typedef struct {
    uint8_t mask;
    uint8_t blinkPhaseOn;
    uint8_t blinkPhaseOff;
} jelzo_mask_t;