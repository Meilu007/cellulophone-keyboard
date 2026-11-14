#include "mp_sdk_gui2.h"
#include "Drawing.h"
#include "mp_midi.h"
#include <vector>
#include <algorithm>

using namespace gmpi;
using namespace gmpi_gui;
using namespace GmpiDrawing;

class CellulophoneKeyboardGui : public SeGuiInvisibleBase
{
    MidiGuiPin pinMidiOut;
    IntGuiPin pinChannel;
    IntGuiPin pinStartNote;
    IntGuiPin pinOctaves;
    StringGuiPin pinIvoryColor;
    StringGuiPin pinEbonyColor;
    StringGuiPin pinPressedColor;
    StringGuiPin pinWhiteImage;
    StringGuiPin pinBlackImage;

    Bitmap whiteKeyBmp;
    Bitmap blackKeyBmp;

    int activeNote = -1;
    int dragMode = 0; // 0 = Aus, 1 = Tasten, 2 = Pitch-Wheel, 3 = Mod-Wheel

    float pitchValue = 0.5f; // 0.0 bis 1.0 (0.5 = Mitte / 0 Cent)
    float modValue = 0.0f;   // 0.0 bis 1.0 (0.0 = Unten / CC 0)

    bool isBlackKey(int note) const
    {
        int n = note % 12;
        return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
    }

    int countWhiteKeys(int startNote, int totalNotes) const
    {
        int count = 0;
        for (int i = 0; i < totalNotes; ++i)
        {
            if (!isBlackKey(startNote + i))
                count++;
        }
        return std::max(1, count);
    }

    void onStyleChanged()
    {
        if (!pinWhiteImage.getValue().empty())
            whiteKeyBmp = GetGraphicsFactory().LoadImageU(pinWhiteImage.getValue());
        else
            whiteKeyBmp = nullptr;

        if (!pinBlackImage.getValue().empty())
            blackKeyBmp = GetGraphicsFactory().LoadImageU(pinBlackImage.getValue());
        else
            blackKeyBmp = nullptr;

        invalidateRect();
    }

    // MIDI-Sendefunktionen
    void sendNoteOn(int note, int velocity)
    {
        if (note < 0 || note > 127) return;
        unsigned char msg;
        msg = static_cast<unsigned char>(0x90 | (pinChannel.getValue() & 0x0F));
        msg = static_cast<unsigned char>(note & 0x7F);
        msg = static_cast<unsigned char>(std::clamp(velocity, 1, 127));
        pinMidiOut.send(msg, 3);
    }

    void sendNoteOff(int note)
    {
        if (note < 0 || note > 127) return;
        unsigned char msg;
        msg = static_cast<unsigned char>(0x80 | (pinChannel.getValue() & 0x0F));
        msg = static_cast<unsigned char>(note & 0x7F);
        msg = 0;
        pinMidiOut.send(msg, 3);
    }

    void sendPitchBend(float normVal)
    {
        int bend = static_cast<int>(std::clamp(normVal, 0.0f, 1.0f) * 16383.0f);
        unsigned char msg;
        msg = static_cast<unsigned char>(0xE0 | (pinChannel.getValue() & 0x0F));
        msg = static_cast<unsigned char>(bend & 0x7F);        // LSB (Fein)
        msg = static_cast<unsigned char>((bend >> 7) & 0x7F); // MSB (Grob)
        pinMidiOut.send(msg, 3);
    }

    void sendModWheel(float normVal)
    {
        int ccVal = static_cast<int>(std::clamp(normVal, 0.0f, 1.0f) * 127.0f);
        unsigned char msg;
        msg = static_cast<unsigned char>(0xB0 | (pinChannel.getValue() & 0x0F));
        msg = 1; // CC 01 (Modulation Wheel Standard)
        msg = static_cast<unsigned char>(ccVal & 0x7F);
        pinMidiOut.send(msg, 3);
    }

    // Geometrie-Aufteilung (Links: Wheel-Gehäuse, Rechts: Tasten)
    void getLayoutRects(Rect& outHousingRect, Rect& outPitchRect, Rect& outModRect, Rect& outKeysRect) const
    {
        auto r = getRect();
        float totalW = r.getWidth();
        float totalH = r.getHeight();

        // Das Wheel-Panel links nimmt ca. 14% der Gesamtbreite ein (begrenzt auf 52-84 Pixel)
        float wheelPanelW = std::clamp(totalW * 0.14f, 52.0f, 84.0f);
        if (totalW < 180.0f) wheelPanelW = totalW * 0.25f;

        outHousingRect = Rect(0.0f, 0.0f, wheelPanelW, totalH);
        outKeysRect = Rect(wheelPanelW, 0.0f, totalW, totalH);

        float padX = wheelPanelW * 0.14f;
        float padY = std::clamp(totalH * 0.08f, 6.0f, 14.0f);
        float gap = padX * 0.8f;
        float singleWheelW = (wheelPanelW - (2.0f * padX) - gap) * 0.5f;

        outPitchRect = Rect(padX, padY, padX + singleWheelW, totalH - padY);
        outModRect = Rect(outPitchRect.right + gap, padY, outPitchRect.right + gap + singleWheelW, totalH - padY);
    }

    int hitTestNote(Point point, const Rect& keysRect, float& outRelativeY)
    {
        float startX = keysRect.left;
        float width = keysRect.getWidth();
        float height = keysRect.getHeight();
        if (width <= 0.f || height <= 0.f) return -1;

        int startNote = std::clamp(pinStartNote.getValue(), 0, 115);
        int octaves = std::clamp(pinOctaves.getValue(), 1, 8);
        int totalNotes = octaves * 12 + 1;
        int whiteCount = countWhiteKeys(startNote, totalNotes);
        float whiteWidth = width / static_cast<float>(whiteCount);
        float blackWidth = whiteWidth * 0.62f;
        float blackHeight = height * 0.62f;

        // 1. Schwarze Tasten prüfen (relativ zu keysRect.left)
        if (point.y <= blackHeight)
        {
            int wIndex = 0;
            for (int i = 0; i < totalNotes; ++i)
            {
                int note = startNote + i;
                if (isBlackKey(note))
                {
                    float bx = startX + (wIndex * whiteWidth) - (blackWidth * 0.5f);
                    if (point.x >= bx && point.x <= bx + blackWidth)
                    {
                        outRelativeY = std::clamp(point.y / blackHeight, 0.05f, 1.0f);
                        return note;
                    }
                }
                else
                {
                    wIndex++;
                }
            }
        }

        // 2. Weiße Tasten prüfen (relativ zu keysRect.left)
        int wIndex = 0;
        for (int i = 0; i < totalNotes; ++i)
        {
            int note = startNote + i;
            if (!isBlackKey(note))
            {
                float wx = startX + (wIndex * whiteWidth);
                if (point.x >= wx && point.x < wx + whiteWidth)
                {
                    outRelativeY = std::clamp(point.y / height, 0.05f, 1.0f);
                    return note;
                }
                wIndex++;
            }
        }
        return -1;
    }

    void drawSingleWheel(Graphics& g, const Rect& slotRect, float normVal, bool isPitch,
                         SolidColorBrush& housingBrush, SolidColorBrush& brassBrush, SolidColorBrush& borderBrush)
    {
        // Vertiefter Bakelit-Schacht
        g.FillRectangle(slotRect, housingBrush);
        g.DrawRectangle(slotRect, borderBrush, 1.2f);

        // Mittellinie bei Pitch-Wheel (0-Punkt-Markierung)
        if (isPitch)
        {
            float midY = slotRect.top + (slotRect.getHeight() * 0.5f);
            g.DrawLine(Point(slotRect.left, midY), Point(slotRect.right, midY), borderBrush, 1.0f);
        }

        // Beweglicher Messing-Schieber / Griffmulde
        float thumbH = std::clamp(slotRect.getHeight() * 0.16f, 8.0f, 18.0f);
        float travel = slotRect.getHeight() - thumbH;
        float thumbTop = slotRect.top + ((1.0f - std::clamp(normVal, 0.0f, 1.0f)) * travel);

        Rect thumbRect(slotRect.left + 1.5f, thumbTop, slotRect.right - 1.5f, thumbTop + thumbH);
        g.FillRectangle(thumbRect, brassBrush);
        g.DrawRectangle(thumbRect, borderBrush, 1.0f);

        // Feine Griff-Rille in der Mitte des Rads
        float grooveY = thumbTop + (thumbH * 0.5f);
        g.DrawLine(Point(thumbRect.left + 2.0f, grooveY), Point(thumbRect.right - 2.0f, grooveY), housingBrush, 1.2f);
    }

public:
    CellulophoneKeyboardGui()
    {
        initializePin(0, pinMidiOut);
        initializePin(1, pinChannel);
        initializePin(2, pinStartNote, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(3, pinOctaves, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(4, pinIvoryColor, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(5, pinEbonyColor, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(6, pinPressedColor, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(7, pinWhiteImage, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(8, pinBlackImage, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
    }

    int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override
    {
        Graphics g(drawingContext);
        Rect housingRect, pitchRect, modRect, keysRect;
        getLayoutRects(housingRect, pitchRect, modRect, keysRect);

        float startX = keysRect.left;
        float width = keysRect.getWidth();
        float height = keysRect.getHeight();

        int startNote = std::clamp(pinStartNote.getValue(), 0, 115);
        int octaves = std::clamp(pinOctaves.getValue(), 1, 8);
        int totalNotes = octaves * 12 + 1;
        int whiteCount = countWhiteKeys(startNote, totalNotes);
        float whiteWidth = width / static_cast<float>(whiteCount);
        float blackWidth = whiteWidth * 0.62f;
        float blackHeight = height * 0.62f;

        auto ivoryBrush = g.CreateSolidColorBrush(Color::FromHexString(pinIvoryColor.getValue()));
        auto ebonyBrush = g.CreateSolidColorBrush(Color::FromHexString(pinEbonyColor.getValue()));
        auto pressedBrush = g.CreateSolidColorBrush(Color::FromHexString(pinPressedColor.getValue()));
        auto borderBrush = g.CreateSolidColorBrush(Color::FromRgb(0x3A2F25));
        auto slotBrush = g.CreateSolidColorBrush(Color::FromRgb(0x120E0B));

        // 1. Linkes Bakelit-Gehäuse für Pitch- & Mod-Wheel zeichnen
        g.FillRectangle(housingRect, ebonyBrush);
        g.DrawRectangle(housingRect, borderBrush, 1.5f);

