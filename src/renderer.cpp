#include "renderer.h"
#include <cmath>
#include <algorithm>

Renderer::Renderer() {}

Renderer::~Renderer() {
    if (sdlRenderer) {
        SDL_DestroyRenderer(sdlRenderer);
    }
}

bool Renderer::init(SDL_Window* window) {
    sdlRenderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!sdlRenderer) {
        return false;
    }

    SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
    SDL_GetRendererOutputSize(sdlRenderer, &windowWidth, &windowHeight);

    return true;
}

void Renderer::clear() {
#ifdef PLATFORM_WINDOWS
    // Magenta color-key: exact-match pixels become transparent via DWM.
    // MUST be fully opaque (a=255) so SDL does not pre-blend it with body
    // pixels before the color-key pass — any partial blend produces purple
    // fringing that the color-key cannot remove.
    SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 255, 255);
#else
    // X11 + compositor: true alpha transparency
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 0);
#endif
    SDL_RenderClear(sdlRenderer);
}

void Renderer::render(const Spider& spider) {
    drawSpiderLegs(spider);
    drawSpiderBody(spider);
    drawEyes(spider);
}

void Renderer::present() {
    SDL_RenderPresent(sdlRenderer);
}

// ---------------------------------------------------------------------------
// drawFilledCircle / drawFilledEllipse / drawThickLine
// All drawing helpers flush their color once and never set alpha < 255 on
// Windows so blended edges cannot produce an intermediate purple.
// On Linux alpha < 255 is fine because the compositor handles it correctly.
// ---------------------------------------------------------------------------

// Opaque wrapper: on Windows force alpha=255 to avoid color-key fringing.
static Color opaqueColor(Color c) {
#ifdef PLATFORM_WINDOWS
    c.a = 255;
#endif
    return c;
}

void Renderer::drawFilledCircle(int cx, int cy, int radius, Color color) {
    color = opaqueColor(color);
    SDL_SetRenderDrawColor(sdlRenderer, color.r, color.g, color.b, color.a);

    for (int dy = -radius; dy <= radius; dy++) {
        int dx = static_cast<int>(std::sqrt(
            static_cast<float>(radius * radius - dy * dy)));
        SDL_RenderDrawLine(sdlRenderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Renderer::drawFilledEllipse(int cx, int cy, int rx, int ry, Color color) {
    color = opaqueColor(color);
    SDL_SetRenderDrawColor(sdlRenderer, color.r, color.g, color.b, color.a);

    for (int dy = -ry; dy <= ry; dy++) {
        float ratio = 1.0f - (static_cast<float>(dy * dy) /
                              static_cast<float>(ry * ry));
        if (ratio < 0.0f) ratio = 0.0f;
        int dx = static_cast<int>(rx * std::sqrt(ratio));
        SDL_RenderDrawLine(sdlRenderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Renderer::drawThickLine(const Vec2& start, const Vec2& end,
                              float thickness, Color color) {
    color = opaqueColor(color);
    SDL_SetRenderDrawColor(sdlRenderer, color.r, color.g, color.b, color.a);

    Vec2 dir = (end - start).normalized();
    Vec2 perp(-dir.y, dir.x);

    int halfThick = static_cast<int>(thickness / 2.0f + 0.5f);

    for (int i = -halfThick; i <= halfThick; i++) {
        float offset = static_cast<float>(i);
        int x1 = static_cast<int>(start.x + perp.x * offset);
        int y1 = static_cast<int>(start.y + perp.y * offset);
        int x2 = static_cast<int>(end.x + perp.x * offset);
        int y2 = static_cast<int>(end.y + perp.y * offset);
        SDL_RenderDrawLine(sdlRenderer, x1, y1, x2, y2);
    }
}

// ---------------------------------------------------------------------------
// Body
// ---------------------------------------------------------------------------

void Renderer::drawSpiderBody(const Spider& spider) {
    Vec2 pos = spider.getPosition();
    float heading = spider.getHeading();
    float pulse = spider.getBodyPulse();
    float bob = spider.getBodyBob();

    // Apply body bob (vertical screen-space offset, legs stay planted)
    pos.y += bob;

    // Body dimensions with breathing pulse
    float breathScale = 1.0f + std::sin(pulse) * 0.02f;
    int bodyRx = static_cast<int>(14.0f * breathScale);
    int bodyRy = static_cast<int>(18.0f * breathScale);

    // Abdomen (larger, behind)
    float abdomenOffset = -12.0f;
    int abdX = static_cast<int>(pos.x + std::cos(heading) * abdomenOffset);
    int abdY = static_cast<int>(pos.y + std::sin(heading) * abdomenOffset);

    // Alpha 255 everywhere — partial alpha blended over magenta produces purple
    // fringing on Windows; the compositor on Linux handles full-alpha fine too.
    Color bodyColor(35, 25, 20, 255);
    Color abdomenColor(45, 32, 25, 255);
    Color highlightColor(65, 50, 40, 255);

    // Draw abdomen
    drawFilledEllipse(abdX, abdY, bodyRx + 3, bodyRy + 4, abdomenColor);
    // Abdomen highlight
    drawFilledEllipse(abdX - 2, abdY - 3, bodyRx - 4, bodyRy - 4, highlightColor);

    // Draw cephalothorax (front body part)
    float cephOffset = 10.0f;
    int cephX = static_cast<int>(pos.x + std::cos(heading) * cephOffset);
    int cephY = static_cast<int>(pos.y + std::sin(heading) * cephOffset);
    drawFilledEllipse(cephX, cephY, bodyRx - 2, bodyRy - 5, bodyColor);

    // Body connection
    int midX = static_cast<int>(pos.x);
    int midY = static_cast<int>(pos.y);
    drawFilledEllipse(midX, midY, bodyRx - 4, bodyRy - 6, bodyColor);

    // Abdomen pattern (subtle markings)
    Color patternColor(55, 40, 30, 255);
    drawFilledEllipse(abdX, abdY - 2, 4, 6, patternColor);
    drawFilledEllipse(abdX, abdY + 4, 3, 4, patternColor);
}

// ---------------------------------------------------------------------------
// Legs
// ---------------------------------------------------------------------------

void Renderer::drawSpiderLegs(const Spider& spider) {
    Vec2 pos = spider.getPosition();
    float heading = spider.getHeading();
    const LegSegment* legs = spider.getLegs();

    Color legColor(40, 30, 22, 255);
    Color legColor2(50, 38, 28, 255);
    Color jointColor(60, 45, 35, 255);

    float cosH = std::cos(heading);
    float sinH = std::sin(heading);

    for (int i = 0; i < Spider::NUM_LEGS; i++) {
        Vec2 footPos = legs[i].position;

        // Leg attachment point on body (coxa)
        int side = (i < 4) ? -1 : 1;
        int idx = i % 4;

        float attachForward = (1.5f - idx) * 5.0f;
        float attachSide = 7.0f * side;

        Vec2 attachWorld(
            pos.x + attachForward * cosH - attachSide * sinH,
            pos.y + attachForward * sinH + attachSide * cosH
        );

        // Calculate knee position using IK-style approach
        Vec2 toFoot = footPos - attachWorld;
        float legLen = toFoot.length();

        Vec2 midPoint = Vec2::lerp(attachWorld, footPos, 0.4f);
        Vec2 legDir = toFoot.normalized();
        Vec2 kneePerp(-legDir.y * side, legDir.x * side);
        float kneeElevation = std::max(10.0f, legLen * 0.35f);
        Vec2 knee = midPoint + kneePerp * kneeElevation * 0.7f +
                    Vec2(0, -kneeElevation * 0.5f);

        float thickness = 2.0f - idx * 0.15f;

        drawThickLine(attachWorld, knee, thickness + 0.8f, legColor);
        drawThickLine(knee, footPos, thickness, legColor2);

        drawFilledCircle(static_cast<int>(knee.x), static_cast<int>(knee.y),
                         static_cast<int>(thickness + 1.5f), jointColor);
        drawFilledCircle(static_cast<int>(footPos.x), static_cast<int>(footPos.y),
                         2, jointColor);
    }
}

// ---------------------------------------------------------------------------
// Eyes
// ---------------------------------------------------------------------------

void Renderer::drawEyes(const Spider& spider) {
    Vec2 pos = spider.getPosition();
    float heading = spider.getHeading();
    SpiderState state = spider.getState();

    float twitch = spider.getMicroTwitch();
    float tCos = std::cos(heading + twitch);
    float tSin = std::sin(heading + twitch);

    float eyeForward = 14.0f;
    float eyeSpread  = 5.0f;

    Color eyeColor(180, 10, 10, 255);
    Color pupilColor(20, 0, 0, 255);
    Color glintColor(255, 200, 200, 255);

    if (state == SpiderState::CHARGE) {
        eyeColor = Color(240, 20, 20, 255);
    } else if (state == SpiderState::OBSERVE || state == SpiderState::ALERT) {
        eyeColor = Color(200, 40, 10, 255);
    }

    for (int eye = 0; eye < 2; eye++) {
        float side = (eye == 0) ? -1.0f : 1.0f;
        float ex = pos.x + tCos * eyeForward + (-tSin) * eyeSpread * side;
        float ey = pos.y + tSin * eyeForward +   tCos  * eyeSpread * side;

        int eyeRadius = 3;
        drawFilledCircle(static_cast<int>(ex), static_cast<int>(ey),
                         eyeRadius, eyeColor);
        drawFilledCircle(static_cast<int>(ex), static_cast<int>(ey),
                         eyeRadius - 1, pupilColor);
        drawFilledCircle(static_cast<int>(ex - 1), static_cast<int>(ey - 1),
                         1, glintColor);
    }

    float eye2Forward = 11.0f;
    float eye2Spread  = 8.0f;
    for (int eye = 0; eye < 2; eye++) {
        float side = (eye == 0) ? -1.0f : 1.0f;
        float ex = pos.x + tCos * eye2Forward + (-tSin) * eye2Spread * side;
        float ey = pos.y + tSin * eye2Forward +   tCos  * eye2Spread * side;
        drawFilledCircle(static_cast<int>(ex), static_cast<int>(ey),
                         2, eyeColor);
    }
}
