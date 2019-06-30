#define LED_COUNT 18

class LED {
    public:
        void init();
        void startup(uint16_t runs, uint32_t wait);
        void showRing();
        void pulse(uint8_t hue, uint8_t sat, uint32_t wait, bool powerdown);
        void pauseRing(int time);
        void ranking(float percent);
        void showCapture();
        void showWin();
    private:
};

enum class LED_Event {startup, capture, ranking, win, error};