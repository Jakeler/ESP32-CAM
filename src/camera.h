#include <Arduino.h>
#include <FS.h>

class CAM {
    public:
        void init();
        void initFS();
        void capture();
        void clearFb();
        bool saveCurrentImage();
        uint16_t getNextImgId();
        String getImgPath(String id);
        size_t getFreeFlash();

        bool checkImgExists(String id);
        File getImgFile(String id);

        bool deleteImg(String id);
        bool wipeFS();
};