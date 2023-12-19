#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN    5
#define LED_COUNT 36

//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ------ Maps of pixels by physical position
static const uint8_t ENDROW = 255;

static const uint8_t Y0[3] = { 14, 34, ENDROW };
static const uint8_t Y1[13] = { 0, 4, 6, 9, 13, 15, 16, 23, 24, 25, 26, 33, ENDROW };
static const uint8_t Y2[11] = { 1, 3, 7, 10, 12, 17, 22, 21, 27, 29, ENDROW };
static const uint8_t Y3[12] = { 2, 5, 8, 11, 18, 19, 20, 30, 31, 28, 32, ENDROW };
static const uint8_t* Y[5] = { Y0, Y1, Y2, Y3, NULL };

static const uint8_t X0[2] = { 1, ENDROW };
static const uint8_t X1[3] = { 0, 2, ENDROW };
static const uint8_t X2[2] = { 3, ENDROW };
static const uint8_t X3[3] = { 4, 5, ENDROW };
static const uint8_t X4[2] = { 6, ENDROW };
static const uint8_t X5[3] = { 7, 8, ENDROW };
static const uint8_t X6[4] = { 9, 10, 11, ENDROW };
static const uint8_t X7[4] = { 12, 13, 14, ENDROW };
static const uint8_t X8[3] = { 16, 17, ENDROW };
static const uint8_t X9[3] = { 15, 18, ENDROW };
static const uint8_t X10[3] = { 22, 19, ENDROW };
static const uint8_t X11[2] = { 23, ENDROW };
static const uint8_t X12[4] = { 24, 21, 20, ENDROW };
static const uint8_t X13[3] = { 25, 30, ENDROW };
static const uint8_t X14[3] = { 29, 31, ENDROW };
static const uint8_t X15[4] = { 26, 27, 28, ENDROW };
static const uint8_t X16[4] = { 32, 33, 34, ENDROW };
static const uint8_t* X[] = { X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11, X12, X13, X14, X15, X16, NULL };

// OMICS letters are 0..24
static const uint8_t OMICS_LAST_LED = 24;
// AI letters are 25..34
//static const uint32_t AI_PURPLE_HSV = 0xadc8e3; // "true" RGB is #4d51e5 = 238deg, 66%, 90% = hsv(a8,a9,e5) but this looks closer to me
static const uint32_t AI_PURPLE_HSV = 0xa8a9e5; 


// PixelAnimator implementations -----------------
class PixelAnimator {
  uint8_t frameDelay{0};
  bool keepRunning{true};
  const uint8_t frameSkip;

  protected:
  PixelAnimator(const uint8_t frameSkip) : frameSkip(frameSkip) {}

  public:

  /**
   * Handles frameSkip/frameDelay, and calls update() when it's time.
   *
   * Returns false when the animation cycle is nearing an end, and the transition
   * to some other animator should begin. Otherwise returns true to say "keep
   * calling me back." This is done by caching the last value returned by update().
   */
  bool nextFrame(uint32_t *scene) {
    if (frameDelay > 0) {
      frameDelay--;
    } else {
      frameDelay = frameSkip;
      keepRunning = update(scene);
    }
    return keepRunning;
  }

  /**
   * Applies this animation's next actual frame to the given scene. Does not necessarily
   * write all pixels, but typically will.
   *
   * Returns false when the animation cycle is nearing an end, and the transition
   * to some other animator should begin. Otherwise returns true to say "keep
   * calling me back."
   */
  virtual bool update(uint32_t *scene);

  /**
   * Resets this animator to its initial settings so it can be used again. This method
   * is also called before first use to ensure consistent behaviour.
   */
  virtual void reset() {
    frameDelay = 0;
    keepRunning = true;
  }
};

class CompoundAnim : public PixelAnimator {
  private:
  uint8_t nAnimators;
  uint8_t currentAnimator{0};
  PixelAnimator **animators;
  
  public:
  CompoundAnim(PixelAnimator *animators[], uint8_t nAnimators)
    : PixelAnimator{0}
    , animators{animators}
    , nAnimators{nAnimators}
  {}

  virtual bool update(uint32_t *scene) {
    if (!animators[currentAnimator % nAnimators]->nextFrame(scene)) {
      currentAnimator++;
      if (currentAnimator >= nAnimators) {
        return false;
      }
    }
    return true;
  }

  virtual void reset() {
    PixelAnimator::reset();
    currentAnimator = 0;
    for (int i = 0; i < nAnimators; i++) {
      animators[i]->reset();
    }
  }
};


// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
class SeriesFill : public PixelAnimator {
  private:
  const uint32_t hsv;
  uint8_t nextLed{0};

  public:
  SeriesFill(uint32_t hsv, uint8_t frameSkip)
    : hsv{hsv}
    , PixelAnimator{frameSkip}
  {}

  virtual bool update(uint32_t *scene) {
    scene[nextLed % LED_COUNT] = hsv;
    nextLed++;
    if (nextLed >= LED_COUNT) {
      // Serial.print("SeriesFill got to pixel ");
      // Serial.println(nextLed);
      return false;
    }
    return true;
  }

  virtual void reset() {
    Serial.println("SeriesFill reset");
    PixelAnimator::reset();
    nextLed = 0;
  }
};

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
class Rainbow : public PixelAnimator {
  private:
  uint32_t firstPixelHue{0};

  public:
  Rainbow(uint8_t frameSkip)
    : PixelAnimator{frameSkip}
    {}

  virtual bool update(uint32_t *scene) {
    for (int i = 0; i < LED_COUNT; i++) {
      uint32_t hue = firstPixelHue + i * 255L / LED_COUNT;
      scene[i] = (hue << 16) | 0xffff;
    }
    firstPixelHue++;
    return firstPixelHue < 3 * 255;
  }

  virtual void reset() {
    PixelAnimator::reset();
    firstPixelHue = 0;
  }
};

class ColorSparkle : public PixelAnimator {
  const uint8_t firstLed;
  const uint8_t lastLed;
  uint16_t firstPixelHue{0};
  int16_t ttl{40};

  public:
  ColorSparkle(uint8_t firstLed, uint8_t lastLed, int frameSkip)
    : firstLed{firstLed}
    , lastLed{lastLed}
    , PixelAnimator{frameSkip}
    {}

  virtual bool update(uint32_t *scene) {
    for (uint8_t c = firstLed; c < lastLed; c++) {
      uint32_t hue = firstPixelHue + c * 255 / (lastLed - firstLed);
      scene[c] = (hue << 16) | 0xffff;
    }
    firstPixelHue += 50;
    ttl--;
    return ttl > 0;
  }

  virtual void reset() {
    PixelAnimator::reset();
    firstPixelHue = 0;
    ttl = 30;
  }
};

class GeomFill : public PixelAnimator {
  private:
  const uint8_t **map;
  const uint32_t hsv;
  uint16_t rownum{0};
  bool done{false};

  public:
  GeomFill(const uint8_t **map, uint32_t hsv, uint8_t frameSkip)
    : map{map}
    , hsv{hsv}
    , PixelAnimator{frameSkip}
    {}

  virtual bool update(uint32_t *scene) {
    if (map[rownum] == NULL) {
      done = true;
      rownum = 0;        
    }
    uint8_t* leds = map[rownum++];
    for (int col = 0; leds[col] != ENDROW; col++) {
      scene[leds[col]] = hsv;
    }
    return !done;
  }

  virtual void reset() {
    PixelAnimator::reset();
    rownum = 0;
    done = false;
  }
};

void fillScene(uint32_t *scene, uint32_t colorHsv) {
  for (int i = 0; i < LED_COUNT; i++) scene[i] = colorHsv;
}
void fillSceneOmics(uint32_t *scene, uint32_t colorHsv) {
  for (int i = 0; i <= OMICS_LAST_LED; i++) scene[i] = colorHsv;
}
void fillSceneAi(uint32_t *scene, uint32_t colorHsv) {
  for (int i = OMICS_LAST_LED + 1; i < LED_COUNT; i++) scene[i] = colorHsv;
}

void showScene(uint32_t *scene) {
    for (int i = 0; i < LED_COUNT; i++) {
      uint32_t hsv = scene[i];
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((hsv >> 8) & 0xff00, (hsv >> 8) & 0xff, hsv & 0xff)));
    }
    strip.show();
}

void transitionDissolve(uint32_t *fromScene, uint32_t *toScene) {
  // Avoid unproductive colour cycling when the "from" or "to" colour is black or white.
  // This is accomplished by syncing the black or white pixel's hue to match the other one
  for (int i = 0; i < LED_COUNT; i++) {
    if ((fromScene[i] & 0x00ff00 == 0) ||  // from white
        (fromScene[i] & 0x0000ff == 0)) {  // from black
      fromScene[i] &= 0x00ffff;
      fromScene[i] |= toScene[i] & 0xff0000;
    }
    if ((toScene[i]   & 0x00ff00 == 0) ||  // to white
        (toScene[i]   & 0x0000ff == 0)) {  // to black
      toScene[i] &= 0x00ffff;
      toScene[i] |= fromScene[i] & 0xff0000;
    }
  }

  const int wait = 8;
  const int steps = 127;
  for (int step = 0; step < steps; step++) {
    uint8_t portion = step * 255 / steps;
    // Serial.print("portion=");
    // Serial.println(portion);
    for (int i = 0; i < LED_COUNT; i++) {
      // if (i == 0 || i == OMICS_LAST_LED + 1) {
      //   Serial.print("from=0x");
      //   Serial.print(fromScene[i], 16);
      //   Serial.print(" to=0x");
      //   Serial.print(toScene[i], 16);
      // }

      uint32_t blendedHsv = lerp(fromScene[i], toScene[i], portion);

      // if (i == 0 || i == OMICS_LAST_LED + 1) {
      //   Serial.print(" lerp=0x");
      //   Serial.println(blendedHsv, 16);
      // }

      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((blendedHsv >> 8) & 0xff00, (blendedHsv >> 8) & 0xff, blendedHsv & 0xff)));
    }
    strip.show();
    delay(wait);
  }

  // show the final scene because we may not have made it all the way to 255 on the last lerp step
  showScene(toScene);
}

void transitionWipe(uint32_t *toScene, uint8_t **geometry, uint32_t frameDelay) {
  for (int rownum = 0; geometry[rownum] != NULL; rownum++) {
    uint8_t* leds = geometry[rownum];
    for (int col = 0; leds[col] != ENDROW; col++) {
      uint16_t ledIndex = leds[col];
      uint32_t hsv = toScene[ledIndex];
      strip.setPixelColor(ledIndex, strip.ColorHSV((hsv >> 8) & 0xff00, (hsv >> 8) & 0xff, hsv & 0xff));
    }
    strip.show();
    delay(frameDelay);
  }
}

uint32_t lerp(uint32_t a, uint32_t b, uint8_t p) {
  uint8_t ah = (a >> 16) & 0xff;
  uint8_t bh = (b >> 16) & 0xff;
  uint8_t as = (a >> 8) & 0xff;
  uint8_t bs = (b >> 8) & 0xff;
  uint8_t av = a & 0xff;
  uint8_t bv = b & 0xff;

  uint8_t h = static_cast<uint8_t>((ah * (255 - p) + bh * p) / 255);
  uint8_t s = static_cast<uint8_t>((as * (255 - p) + bs * p) / 255);
  uint8_t v = static_cast<uint8_t>((av * (255 - p) + bv * p) / 255);

  return
    (static_cast<uint32_t>(h) << 16) |
    (static_cast<uint32_t>(s) << 8) |
    static_cast<uint32_t>(v);
}

// Main Program -------------------------------------------------

static PixelAnimator *animBgrSeriesFill;
static PixelAnimator *animRainbow;
static PixelAnimator *animGeomFillY;
static PixelAnimator *animAiColorSparkle;

static PixelAnimator *currentAnim = NULL;

// Delay between frames. Changing this will globally alter how fast effects and transitions run.
static const uint8_t FRAME_PERIOD = 2;

// Time to wait on the logo in its proper form before starting a new animation cycle
static const uint16_t LOGO_DWELL_MILLIS = 2000; // 15000;

// two buffers for the scene, both in packed HSV 8,8,8
uint32_t scene1[LED_COUNT];
uint32_t scene2[LED_COUNT];

enum Mode { AUTO, SINGLE };
Mode mode = AUTO;

void setup() {
  strip.begin();
  strip.show(); // ensure pixels don't come on at 100% white (mega power draw)
  strip.setBrightness(50); // (max = 255)

  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("creating animators");

  PixelAnimator *sfComponents[3] = {
    new SeriesFill(0xaaffff, 10), // blue
    new SeriesFill(0x55ffff, 10), // green
    new SeriesFill(0x00ffff, 10)  // red
  };
  animBgrSeriesFill = new CompoundAnim(sfComponents, 3);

  animRainbow = new Rainbow(5);

  PixelAnimator *gfComponents[5] = {
    new GeomFill(Y, 0xccffff, 20),
    new GeomFill(Y, 0x99ffff, 20),
    new GeomFill(Y, 0x66ffff, 20),
    new GeomFill(Y, 0x33ffff, 20),
    new GeomFill(Y, 0x00ffff, 20)
  };
  animGeomFillY = new CompoundAnim(gfComponents, 5);

  animAiColorSparkle = new ColorSparkle(OMICS_LAST_LED + 1, LED_COUNT, 8);

  Serial.println("init() finished");
}

// ------------------------------------------
//         MAIN LOOP
// ------------------------------------------
void loop() {
  Serial.println("starting loop");

  if (Serial.available() > 0) {
    processSerial();
  }

  if (mode == AUTO) {
    if (currentAnim == NULL) {
      // choose random effect and run it on scene 1
      switch (random(4)) {
        case 0:
          currentAnim = animRainbow;
          break;
        case 1:
          currentAnim = animAiColorSparkle;
          break;
        case 2:
          currentAnim = animBgrSeriesFill;
          break;
        case 3:
          currentAnim = animGeomFillY;
          break;
      }
      currentAnim->reset();
    }

    while (currentAnim->nextFrame(scene1)) {
      showScene(scene1);
      delay(FRAME_PERIOD);

      if (Serial.available() > 0) {
        break;
      }
    }

    // TODO transition and animate simultaneously
    // choose random wipe to reveal scene 2 (the proper logo colours)
    fillSceneOmics(scene2, 0xa800ff);
    fillSceneAi(scene2, AI_PURPLE_HSV);
    switch (random(4)) {
      case 0:
        transitionWipe(scene2, Y, 90);
        break;
      case 1:
        transitionWipe(scene2, X, 15);
        break;
      case 2:
        transitionDissolve(scene1, scene2);
        break;
      case 3:
        fillScene(scene1, 0xa800ff); // dissolve from white
        transitionDissolve(scene1, scene2);
        break;
      // case 4:
      //   aiColorSparkle(scene1, 50);
      //   break;
    }

    currentAnim = NULL;

    // stay on the logo for a while
    delay(LOGO_DWELL_MILLIS);
  }
}


// ------------ Serial Console stuff ------------
int inputNumber = -1;
uint32_t inputHsv = 0;

void processSerial() {
  char ch = Serial.read();
  Serial.print(ch);

  if (ch >= '0' && ch <= '9') {
    mode = SINGLE;
    if (inputNumber < 0) { inputNumber = 0; }
    inputNumber = (10 * inputNumber) + (ch - '0');
  } else if (ch == 'h') {
    inputHsv &= 0x00ffff;
    inputHsv |= (static_cast<uint32_t>(inputNumber) & 0xff) << 16;
    Serial.println(inputHsv, 16);
    fillSceneAi(scene1, inputHsv);
    showScene(scene1);
  } else if (ch == 's') {
    inputHsv &= 0xff00ff;
    inputHsv |= (static_cast<uint32_t>(inputNumber) & 0xff) << 8;
    Serial.println(inputHsv, 16);
    fillSceneAi(scene1, inputHsv);
    showScene(scene1);
  } else if (ch == 'v') {
    inputHsv &= 0xffff00;
    inputHsv |= (static_cast<uint32_t>(inputNumber) & 0xff);
    Serial.println(inputHsv, 16);
    fillSceneAi(scene1, inputHsv);
    showScene(scene1);
  } else if (ch == 'a') {
    mode = AUTO;
  }

  if (ch == '\n') {
    inputNumber = -1;
    Serial.print("> ");
  }
}
