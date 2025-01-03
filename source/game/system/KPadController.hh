#pragma once

#include "game/system/GhostFile.hh"

#include <egg/math/Vector.hh>

namespace System {

enum class ControlSource {
    Unknown = -1,
    Core = 0,      // WiiMote
    Freestyle = 1, // WiiMote + Nunchuk
    Classic = 2,
    Gamecube = 3,
    Ghost = 4,
    AI = 5,
    Host = 6, // Added in Kinoko, represents an external program
};

enum class Trick {
    None = 0,
    Up = 1,
    Down = 2,
    Left = 3,
    Right = 4,
};

/// @brief Represents a set of controller inputs.
struct RaceInputState {
    RaceInputState();
    virtual ~RaceInputState() {}

    void reset();

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isButtonsValid() const;
    [[nodiscard]] bool isStickValid(f32 stick) const;
    [[nodiscard]] bool isTrickValid() const;

    [[nodiscard]] bool accelerate() const;
    [[nodiscard]] bool brake() const;
    [[nodiscard]] bool item() const;
    [[nodiscard]] bool drift() const;
    [[nodiscard]] bool trickUp() const;
    [[nodiscard]] bool trickDown() const;

    u16 buttons;
    u16 buttonsRaw;
    EGG::Vector2f stick;
    u8 stickXRaw;
    u8 stickYRaw;
    Trick trick;
    u8 trickRaw;
};

/// @brief Represents a stream of button inputs from a ghost file.
struct KPadGhostButtonsStream {
    KPadGhostButtonsStream();
    ~KPadGhostButtonsStream();

    [[nodiscard]] virtual u8 readFrame();
    [[nodiscard]] virtual bool readIsNewSequence() const;
    [[nodiscard]] virtual u8 readVal() const;

    EGG::RamStream buffer;
    u32 currentSequence;
    u16 readSequenceFrames;
    u32 state;
};

/// @brief A specialized stream for button presses (not tricks).
/// @details Reads in the status for acceleration, braking, item usage, and drifting. The button
/// tuples take the following form:
/// Bitmask  | Description
///------------- | -------------
/// 0x01  | **Accelerating**
/// 0x02  | **Braking/Drifting**
/// 0x04  | **Item usage**
/// 0x08  | Set if braking/drifting pressed after pressing accelerating
/// @warning When bitmask 0x08 is set, the game will register a hop regardless of whether or
/// not the acceleration button is pressed. This can lead to "successful" synchronization of
/// ghosts which could not have been created legitimately in the first place.
struct KPadGhostFaceButtonsStream : public KPadGhostButtonsStream {
    KPadGhostFaceButtonsStream();
    ~KPadGhostFaceButtonsStream();
};

/// @brief A specialized stream for the analog stick.
/// Direction tuples take the following form:
/// Bitmask  | Description
///------------- | -------------
/// 0x0F  | Up/Down (0xE = Up, 0x0 = Down, 0x7 = Neutral)
/// 0xF0  | Left/Right (0xE0 = Right, 0x00 = Left, 0x70 = Neutral)
struct KPadGhostDirectionButtonsStream : public KPadGhostButtonsStream {
    KPadGhostDirectionButtonsStream();
    ~KPadGhostDirectionButtonsStream();
};

/// @brief A specialized stream for D-Pad inputs for tricking and wheeling.
/// Trick tuples take the following form:
/// Bitmask  | Description
///------------- | -------------
/// 0x0F  | The upper four bits of the tuple's duration, forming a 12-bit integer.
/// 0x70  | 0x00 = No trick, 0x10 = Up/Wheelie, 0x20 = Down, 0x30 = Left, 0x40 = Right
struct KPadGhostTrickButtonsStream : public KPadGhostButtonsStream {
    KPadGhostTrickButtonsStream();
    ~KPadGhostTrickButtonsStream();

    [[nodiscard]] bool readIsNewSequence() const override;
    [[nodiscard]] u8 readVal() const override;
};

/// @brief An abstraction for a controller object. It is associated with an input state.
class KPadController {
public:
    KPadController();
    virtual ~KPadController() {}

    [[nodiscard]] virtual ControlSource controlSource() const;
    virtual void reset(bool /*driftIsAuto*/) {}
    virtual void calcImpl() {}

    void calc();

    [[nodiscard]] const RaceInputState &raceInputState() const;

    void setDriftIsAuto(bool driftIsAuto);

    [[nodiscard]] bool driftIsAuto() const;

protected:
    RaceInputState m_raceInputState; ///< The current inputs from this controller.
    bool m_connected;                ///< Whether the controller is active.
    bool m_driftIsAuto;              ///< True for auto transmission, false for manual.
};

/// @brief The abstraction of a controller object but for ghost playback.
/// @details When playing back ghosts, their input state is managed by this class.
class KPadGhostController : public KPadController {
public:
    KPadGhostController();
    ~KPadGhostController() override;

    [[nodiscard]] ControlSource controlSource() const override;
    void reset(bool driftIsAuto) override;

    void readGhostBuffer(const u8 *buffer, bool driftIsAuto);

    void calcImpl() override;

    void setAcceptingInputs(bool set);

private:
    const u8 *m_ghostBuffer;
    std::array<KPadGhostButtonsStream *, 3> m_buttonsStreams;
    bool m_acceptingInputs;
};

/// @brief The abstraction of a controller object but for external usage.
/// @details The input state is managed externally by programs interfacing with Kinoko.
class KPadHostController : public KPadController {
public:
    KPadHostController();
    ~KPadHostController() override;

    [[nodiscard]] ControlSource controlSource() const override;
    void reset(bool driftIsAuto) override;

    bool setInputs(const RaceInputState &state);
    bool setInputs(u16 buttons, const EGG::Vector2f &stick, Trick trick);
    bool setInputs(u16 buttons, f32 stickX, f32 stickY, Trick trick);
    bool setInputsRawStick(u16 buttons, u8 stickXRaw, u8 stickYRaw, Trick trick);
    bool setInputsRawStickZeroCenter(u16 buttons, s8 stickXRaw, s8 stickYRaw, Trick trick);
};

class KPad {
public:
    KPad();
    ~KPad();

    void calc();
    void reset();

    [[nodiscard]] const RaceInputState &currentState() const;
    [[nodiscard]] const RaceInputState &lastState() const;
    [[nodiscard]] bool driftIsAuto() const;

protected:
    KPadController *m_controller;
    RaceInputState m_currentInputState;
    RaceInputState m_lastInputState; ///< Used to determine changes in input state.
};

/// @brief A specialized KPad for player input, as opposed to CPU players for example.
class KPadPlayer : public KPad {
public:
    KPadPlayer();
    ~KPadPlayer();

    void setGhostController(KPadGhostController *controller, const u8 *inputs, bool driftIsAuto);
    void setHostController(KPadHostController *controller, bool driftIsAuto);

    void startGhostProxy(); ///< Signals to start reading ghost data after fade-in.
    void endGhostProxy();   ///< Signals to stop reading ghost data after race completion.

private:
    u8 m_ghostBuffer[RKG_UNCOMPRESSED_INPUT_DATA_SECTION_SIZE];
};

} // namespace System
