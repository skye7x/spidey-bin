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
    // Magenta color key: exact-match pixels become transparent
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

    // Dark brown/black body
    Color bodyColor(35, 25, 20, 240);
    Color abdomenColor(45, 32, 25, 240);
    Color highlightColor(65, 50, 40, 180);

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
    Color patternColor(55, 40, 30, 120);
    drawFilledEllipse(abdX, abdY - 2, 4, 6, patternColor);
    drawFilledEllipse(abdX, abdY + 4, 3, 4, patternColor);
}

void Renderer::drawSpiderLegs(const Spider& spider) {
    Vec2 pos = spider.getPosition();
    float heading = spider.getHeading();
    const LegSegment* legs = spider.getLegs();

    Color legColor(40, 30, 22, 230);
    Color legColor2(50, 38, 28, 210);
    Color jointColor(60, 45, 35, 220);

    float cosH = std::cos(heading);
    float sinH = std::sin(heading);

    for (int i = 0; i < Spider::NUM_LEGS; i++) {
        Vec2 footPos = legs[i].position;

        // Leg attachment point on body (coxa)
        int side = (i < 4) ? -1 : 1;
        int idx = i % 4;

        // Attachment points along body sides, evenly spaced front to back
        float attachForward = (1.5f - idx) * 5.0f;  // front legs attach more forward
        float attachSide = 7.0f * side;

        Vec2 attachWorld(
            pos.x + attachForward * cosH - attachSide * sinH,
            pos.y + attachForward * sinH + attachSide * cosH
        );

        // Calculate knee position using IK-style approach
        Vec2 toFoot = footPos - attachWorld;
        float legLen = toFoot.length();

        // Knee bends UP and outward (like real spider legs)
        Vec2 midPoint = Vec2::lerp(attachWorld, footPos, 0.4f);
        Vec2 legDir = toFoot.normalized();
        // Perpendicular direction for knee bend (outward + up)
        Vec2 kneePerp(-legDir.y * side, legDir.x * side);
        // Knee height proportional to leg length, creating an arch
        float kneeElevation = std::max(10.0f, legLen * 0.35f);
        // Add upward bias (negative Y = up on screen)
        Vec2 knee = midPoint + kneePerp * kneeElevation * 0.7f + Vec2(0, -kneeElevation * 0.5f);

        // Leg thickness: front legs slightly thicker
        float thickness = 2.0f - idx * 0.15f;

        // Draw femur (body to knee)
        drawThickLine(attachWorld, knee, thickness + 0.8f, legColor);
        // Draw tibia (knee to foot)
        drawThickLine(knee, footPos, thickness, legColor2);

        // Joint dot at knee
        drawFilledCircle(static_cast<int>(knee.x), static_cast<int>(knee.y),
                        static_cast<int>(thickness + 1.5f), jointColor);

        // Small foot dot
        drawFilledCircle(static_cast<int>(footPos.x), static_cast<int>(footPos.y),
                        2, jointColor);
    }
}

void Renderer::drawEyes(const Spider& spider) {
    Vec2 pos = spider.getPosition();
    float heading = spider.getHeading();
    SpiderState state = spider.getState();

    float cosH = std::cos(heading);
    float sinH = std::sin(heading);

    float twitch = spider.getMicroTwitch();
    float tCos = std::cos(heading + twitch);
    float tSin = std::sin(heading + twitch);

    // Main eyes (front pair, larger)
    float eyeForward = 14.0f;
    float eyeSpread = 5.0f;

    Color eyeColor(180, 10, 10, 240);      // Red eyes
    Color pupilColor(20, 0, 0, 255);
    Color glintColor(255, 200, 200, 200);

    if (state == SpiderState::CHARGE) {
        eyeColor = Color(240, 20, 20, 255);  // Brighter when charging
    } else if (state == SpiderState::OBSERVE || state == SpiderState::ALERT) {
        eyeColor = Color(200, 40, 10, 240);
    }

    // Position eyes (twitched heading for caution jitter)
    for (int eye = 0; eye < 2; eye++) {
        float side = (eye == 0) ? -1.0f : 1.0f;

        float ex = pos.x + tCos * eyeForward + (-tSin) * eyeSpread * side;
        float ey = pos.y + tSin * eyeForward + tCos * eyeSpread * side;

        int eyeRadius = 3;
        drawFilledCircle(static_cast<int>(ex), static_cast<int>(ey), eyeRadius, eyeColor);
        drawFilledCircle(static_cast<int>(ex), static_cast<int>(ey), eyeRadius - 1, pupilColor);

        // Eye glint
        drawFilledCircle(static_cast<int>(ex - 1), static_cast<int>(ey - 1), 1, glintColor);
    }

    // Secondary eyes (smaller, behind main eyes)
    float eye2Forward = 11.0f;
    float eye2Spread = 8.0f;
    for (int eye = 0; eye < 2; eye++) {
        float side = (eye == 0) ? -1.0f : 1.0f;
        float ex = pos.x + tCos * eye2Forward + (-tSin) * eye2Spread * side;
        float ey = pos.y + tSin * eye2Forward + tCos * eye2Spread * side;
        drawFilledCircle(static_cast<int>(ex), static_cast<int>(ey), 2, eyeColor);
    }
}

void Renderer::drawFilledCircle(int cx, int cy, int radius, Color color) {
    SDL_SetRenderDrawColor(sdlRenderer, color.r, color.g, color.b, color.a);

    for (int dy = -radius; dy <= radius; dy++) {
        int dx = static_cast<int>(std::sqrt(radius * radius - dy * dy));
        SDL_RenderDrawLine(sdlRenderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Renderer::drawFilledEllipse(int cx, int cy, int rx, int ry, Color color) {
    SDL_SetRenderDrawColor(sdlRenderer, color.r, color.g, color.b, color.a);

    for (int dy = -ry; dy <= ry; dy++) {
        float ratio = 1.0f - (static_cast<float>(dy * dy) / static_cast<float>(ry * ry));
        if (ratio < 0.0f) ratio = 0.0f;
        int dx = static_cast<int>(rx * std::sqrt(ratio));
        SDL_RenderDrawLine(sdlRenderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Renderer::drawThickLine(const Vec2& start, const Vec2& end, float thickness, Color color) {
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
