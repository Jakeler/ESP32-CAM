#include <Arduino.h>

class MQTT {
    public:
        MQTT();
        int currentScore;
        int maxScore;
        void connect();
        void publishScore();
        void handle();
    private:
        void incomingCallback(char* topic, uint8_t* payload, unsigned int length);
};