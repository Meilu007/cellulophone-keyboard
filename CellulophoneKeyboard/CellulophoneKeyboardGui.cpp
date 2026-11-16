#define NOMINMAX
#include "mp_sdk_gui2.h"
#include "mp_midi.h"
#include "Drawing.h"
#include "unicode_conversion.h"
#include <algorithm>

using namespace gmpi;
using namespace gmpi_gui;
using namespace GmpiDrawing;
using namespace JmUnicodeConversions;

class CellulophoneKeyboardGui : public MpGuiGfxBase
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
    int dragMode = 0;

    float pitchValue = 0.5f;
    float modValue = 0.0f;

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
            whiteKeyBmp = GetGraphicsFactory().LoadImageU(WStringToUtf8(pinWhiteImage.getValue()));
        else
            whiteKeyBmp = nullptr;

        if (!pinBlackImage.getValue().empty())
            blackKeyBmp = GetGraphicsFactory().LoadImageU(WStringToUtf8(pinBlackImage.getValue()));
        else
            blackKeyBmp = nullptr;

        invalidateRect();
    }

    void sendNoteOn(int note, int velocity)
    {
        if (note < 0 || note > 127) return;
        const unsigned char msg[] = {
            static_cast<unsigned char>(0x90 | (pinChannel.getValue() & 0x0F)),
            static_cast<unsigned char>(note & 0x7F),
            static_cast<unsigned char>(std::clamp(velocity, 1, 127))
        };
        pinMidiOut.send(msg, 3);
    }

    void sendNoteOff(int note)
    {
        if (note < 0 || note > 127) return;
        const unsigned char msg[] = {
            static_cast<unsigned char>(0x80 | (pinChannel.getValue() & 0x0F)),
            static_cast<unsigned char>(note & 0x7F),
            0
        };
        pinMidiOut.send(msg, 3);
    }

    void sendPitchBend(float normVal)
    {
        int bend = static_cast<int>(std::clamp(normVal, 0.0f, 1.0f) * 16383.0f);
        const unsigned char msg[] = {
            static_cast<unsigned char>(0xE0 | (pinChannel.getValue() & 0x0F)),
            static_cast<unsigned char>(bend & 0x7F),
            static_cast<unsigned char>((bend >> 7) & 0x7F)
        };
        pinMidiOut.send(msg, 3);
    }

    void sendModWheel(float normVal)
    {
        int ccVal = static_cast<int>(std::clamp(normVal, 0.0f, 1.0f) * 127.0f);
        const unsigned char msg[] = {
            static_cast<unsigned char>(0xB0 | (pinChannel.getValue() & 0x0F)),
            1,
            static_cast<unsigned char>(ccVal & 0x7F)
        };
        pinMidiOut.send(msg, 3);
    }

    void getLayoutRects(Rect& outHousingRect, Rect& outPitchRect, Rect& outModRect, Rect& outKeysRect)
    {
        auto r = getRect();
        float totalW = r.getWidth();
        float totalH = r.getHeight();

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
        g.FillRectangle(slotRect, housingBrush);
        g.DrawRectangle(slotRect, borderBrush, 1.2f);

        if (isPitch)
        {
            float midY = slotRect.top + (slotRect.getHeight() * 0.5f);
            g.DrawLine(Point(slotRect.left, midY), Point(slotRect.right, midY), borderBrush, 1.0f);
        }

        float thumbH = std::clamp(slotRect.getHeight() * 0.16f, 8.0f, 18.0f);
        float travel = slotRect.getHeight() - thumbH;
        float thumbTop = slotRect.top + ((1.0f - std::clamp(normVal, 0.0f, 1.0f)) * travel);

        Rect thumbRect(slotRect.left + 1.5f, thumbTop, slotRect.right - 1.5f, thumbTop + thumbH);
        g.FillRectangle(thumbRect, brassBrush);
        g.DrawRectangle(thumbRect, borderBrush, 1.0f);

        float grooveY = thumbTop + (thumbH * 0.5f);
        g.DrawLine(Point(thumbRect.left + 2.0f, grooveY), Point(thumbRect.right - 2.0f, grooveY), housingBrush, 1.2f);
    }

public:
    CellulophoneKeyboardGui()
    {
        initializePin(pinMidiOut);
        initializePin(pinChannel);
        initializePin(pinStartNote, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(pinOctaves, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(pinIvoryColor, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(pinEbonyColor, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(pinPressedColor, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(pinWhiteImage, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
        initializePin(pinBlackImage, static_cast<MpGuiBaseMemberPtr2>(&CellulophoneKeyboardGui::onStyleChanged));
    }

    int32_t MP_STDCALL initialize() override
    {
        auto res = MpGuiGfxBase::initialize();
        onStyleChanged();
        return res;
    }

    int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override
    {
        returnDesiredSize->width = 360.0f;
        returnDesiredSize->height = 75.0f;
        return MP_OK;
    }

    int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override
    {
        auto res = MpGuiGfxBase::arrange(finalRect);
        invalidateRect();
        return res;
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

        std::wstring ivoryHex = pinIvoryColor.getValue().empty() ? L"FFF4EAD4" : pinIvoryColor.getValue();
        std::wstring ebonyHex = pinEbonyColor.getValue().empty() ? L"FF1F1914" : pinEbonyColor.getValue();
        std::wstring pressedHex = pinPressedColor.getValue().empty() ? L"FFE6C280" : pinPressedColor.getValue();

        auto ivoryBrush = g.CreateSolidColorBrush(Color::FromHexString(ivoryHex));
        auto ebonyBrush = g.CreateSolidColorBrush(Color::FromHexString(ebonyHex));
        auto pressedBrush = g.CreateSolidColorBrush(Color::FromHexString(pressedHex));
        auto borderBrush = g.CreateSolidColorBrush(Color::FromRgb(0x3A2F25));
        auto slotBrush = g.CreateSolidColorBrush(Color::FromRgb(0x120E0B));

        g.FillRectangle(housingRect, ebonyBrush);
        g.DrawRectangle(housingRect, borderBrush, 1.5f);
drawSingleWheel(g, pitchRect, pitchValue, true, slotBrush, pressedBrush, borderBrush);
drawSingleWheel(g, modRect, modValue, false, slotBrush, pressedBrush, borderBrush);
int wIndex = 0;
for (int i = 0; i < totalNotes; ++i)
{
int note = startNote + i;
if (!isBlackKey(note))
{
Rect keyRect(startX + (wIndex * whiteWidth), 0.f, startX + ((wIndex + 1) * whiteWidth), height);
if (whiteKeyBmp)
g.DrawBitmap(whiteKeyBmp, keyRect, Rect(0.f, 0.f, (float)whiteKeyBmp.GetSize().width, (float)whiteKeyBmp.GetSize().height));
else
g.FillRectangle(keyRect, (note == activeNote) ? pressedBrush : ivoryBrush);
g.DrawRectangle(keyRect, borderBrush, 1.0f);
wIndex++;
}
}
wIndex = 0;
for (int i = 0; i < totalNotes; ++i)
{
int note = startNote + i;
if (isBlackKey(note))
{
float bx = startX + (wIndex * whiteWidth) - (blackWidth * 0.5f);
Rect keyRect(bx, 0.f, bx + blackWidth, blackHeight);
if (blackKeyBmp)
g.DrawBitmap(blackKeyBmp, keyRect, Rect(0.f, 0.f, (float)blackKeyBmp.GetSize().width, (float)blackKeyBmp.GetSize().height));
else
g.FillRectangle(keyRect, (note == activeNote) ? pressedBrush : ebonyBrush);
g.DrawRectangle(keyRect, borderBrush, 1.0f);
}
else
{
wIndex++;
}
}
return MP_OK;
}
int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
{
setCapture();
Rect housingRect, pitchRect, modRect, keysRect;
getLayoutRects(housingRect, pitchRect, modRect, keysRect);
if (point.x >= pitchRect.left - 2.0f && point.x <= pitchRect.right + 2.0f)
{
dragMode = 2;
float rel = 1.0f - ((point.y - pitchRect.top) / std::max(1.0f, pitchRect.getHeight()));
pitchValue = std::clamp(rel, 0.0f, 1.0f);
sendPitchBend(pitchValue);
invalidateRect();
return MP_OK;
}
if (point.x >= modRect.left - 2.0f && point.x <= modRect.right + 2.0f)
{
dragMode = 3;
float rel = 1.0f - ((point.y - modRect.top) / std::max(1.0f, modRect.getHeight()));
modValue = std::clamp(rel, 0.0f, 1.0f);
sendModWheel(modValue);
invalidateRect();
return MP_OK;
}
if (point.x >= keysRect.left)
{
dragMode = 1;
float relY = 0.5f;
int hit = hitTestNote(point, keysRect, relY);
if (hit != -1)
{
activeNote = hit;
int velocity = static_cast<int>(15 + relY * 112.0f);
sendNoteOn(activeNote, velocity);
invalidateRect();
}
}
return MP_OK;
}
int32_t MP_STDCALL onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
{
if (dragMode == 0) return MP_OK;
Rect housingRect, pitchRect, modRect, keysRect;
getLayoutRects(housingRect, pitchRect, modRect, keysRect);
if (dragMode == 2)
{
float rel = 1.0f - ((point.y - pitchRect.top) / std::max(1.0f, pitchRect.getHeight()));
pitchValue = std::clamp(rel, 0.0f, 1.0f);
sendPitchBend(pitchValue);
invalidateRect();
}
else if (dragMode == 3)
{
float rel = 1.0f - ((point.y - modRect.top) / std::max(1.0f, modRect.getHeight()));
modValue = std::clamp(rel, 0.0f, 1.0f);
sendModWheel(modValue);
invalidateRect();
}
else if (dragMode == 1)
{
float relY = 0.5f;
int hit = hitTestNote(point, keysRect, relY);
if (hit != activeNote)
{
if (activeNote != -1) sendNoteOff(activeNote);
activeNote = hit;
if (activeNote != -1)
{
int velocity = static_cast<int>(15 + relY * 112.0f);

sendNoteOn(activeNote, velocity);
}
invalidateRect();
}
}
return MP_OK;
}
int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
{
releaseCapture();
if (dragMode == 2)
{
pitchValue = 0.5f;
sendPitchBend(pitchValue);
invalidateRect();
}
else if (dragMode == 1 && activeNote != -1)
{
sendNoteOff(activeNote);
activeNote = -1;
invalidateRect();
}
dragMode = 0;
return MP_OK;
}
int32_t MP_STDCALL onMouseWheel(int32_t flags, int32_t delta, GmpiDrawing_API::MP1_POINT point) override
{
if (activeNote != -1)
{
sendNoteOff(activeNote);
activeNote = -1;
}
int currentStart = pinStartNote.getValue();
int step = (delta > 0) ? 12 : -12;
int newStart = std::clamp(currentStart + step, 12, 96);
if (newStart != currentStart)
{
pinStartNote = newStart;
invalidateRect();
}
return MP_OK;
}
};
namespace
{
namespace
{
    auto r = gmpi::Register<CellulophoneKeyboardGui>::withId(L"Cellulophone_Vintage_Keyboard");
}

}

