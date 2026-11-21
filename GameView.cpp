#include "GameView.h"

#include <Application.h>
#include <Bitmap.h>
#include <ByteOrder.h>
#include <Entry.h>
#include <Message.h>
#include <Path.h>
#include <Roster.h>
#include <SimpleGameSound.h>
#include <TranslationUtils.h>
#include <View.h>

#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <time.h>

enum { MSG_FRAME = 'fram' };

// ---------------------------------------------------------------------------
// Âm thanh: tạo PCM ngay trong code bằng BSimpleGameSound (không cần file nhạc).
// ---------------------------------------------------------------------------
namespace {

enum {
	SFX_SHOOT = 0,
	SFX_HIT,
	SFX_BOOM,
	SFX_POWER,
	SFX_HURT,
	SFX_WAVE,
	SFX_BOSS,
	SFX_GAMEOVER,
	SFX_COUNT
};

const int32 kSfxVoices = 3;
const float kSfxRate = 22050.0f;

struct SfxVoice {
	BSimpleGameSound* sound;
	uint8* data;
};

SfxVoice sSfx[SFX_COUNT][kSfxVoices];
int32 sSfxCursor[SFX_COUNT];
bool sSfxInited = false;
bool sSfxMuted = false;
uint32 sSfxSeed = 0x9e3779b9;

float
SfxNoise()
{
	sSfxSeed = sSfxSeed * 1103515245u + 12345u;
	return ((float)((sSfxSeed >> 16) & 0x7fff) / 16383.5f) - 1.0f;
}

uint8*
MakeSfx(int32 kind, int32 frames)
{
	uint8* data = new uint8[frames];
	for (int32 i = 0; i < frames; i++) {
		float p = i / (float)frames;
		float t = i / kSfxRate;
		float env = 1.0f - p;
		float sample = 0.0f;

		switch (kind) {
			case SFX_SHOOT: {
				float freq = 980.0f - 640.0f * p;
				env = powf(env, 1.5f);
				sample = sinf(6.2831853f * freq * t) * 0.75f
					+ SfxNoise() * 0.25f;
				break;
			}
			case SFX_HIT: {
				env = powf(env, 2.0f);
				sample = SfxNoise() * 0.8f
					+ sinf(6.2831853f * (280.0f + 220.0f * p) * t) * 0.2f;
				break;
			}
			case SFX_BOOM: {
				env = powf(env, 1.8f);
				sample = SfxNoise() * (0.75f - 0.35f * p)
					+ sinf(6.2831853f * (110.0f - 60.0f * p) * t) * 0.5f;
				break;
			}
			case SFX_POWER: {
				int32 note = (int32)(p * 4.0f);
				float freq = 440.0f * powf(1.2599f, (float)note);
				env = 0.55f + 0.45f * sinf(p * 3.14159f);
				sample = sinf(6.2831853f * freq * t) * 0.7f
					+ sinf(6.2831853f * freq * 2.0f * t) * 0.25f;
				break;
			}
			case SFX_HURT: {
				env = powf(env, 1.2f);
				sample = SfxNoise() * 0.45f
					+ sinf(6.2831853f * (420.0f - 300.0f * p) * t) * 0.55f;
				break;
			}
			case SFX_WAVE: {
				float freq = p < 0.5f ? 523.25f : 783.99f;
				env = 0.35f + 0.65f * sinf(p * 3.14159f);
				sample = sinf(6.2831853f * freq * t) * 0.6f
					+ sinf(6.2831853f * freq * 1.5f * t) * 0.3f;
				break;
			}
			case SFX_BOSS: {
				float freq = 130.0f + 40.0f * sinf(t * 18.0f);
				env = 0.5f + 0.5f * sinf(p * 3.14159f);
				sample = sinf(6.2831853f * freq * t) * 0.7f
					+ SfxNoise() * 0.2f;
				break;
			}
			case SFX_GAMEOVER: {
				float freq = 392.0f - 300.0f * p;
				env = powf(1.0f - p, 1.3f);
				sample = sinf(6.2831853f * freq * t) * 0.65f
					+ sinf(6.2831853f * freq * 0.5f * t) * 0.3f;
				break;
			}
		}

		int32 value = 128 + (int32)(sample * env * 110.0f);
		if (value < 0) value = 0;
		if (value > 255) value = 255;
		data[i] = (uint8)value;
	}
	return data;
}

void
InitSfxEngine()
{
	if (sSfxInited) return;
	sSfxInited = true;

	static const int32 lengths[SFX_COUNT] = {
		2205, 2205, 8820, 8820, 6615, 11025, 13230, 22050
	};

	gs_audio_format format;
	format.frame_rate = kSfxRate;
	format.channel_count = 1;
	format.format = gs_audio_format::B_GS_U8;
	format.byte_order = B_HOST_IS_LENDIAN;
	format.buffer_size = 0;

	for (int32 kind = 0; kind < SFX_COUNT; kind++) {
		for (int32 voice = 0; voice < kSfxVoices; voice++) {
			sSfx[kind][voice].data = NULL;
			sSfx[kind][voice].sound = NULL;

			uint8* data = MakeSfx(kind, lengths[kind]);
			BSimpleGameSound* sound = new BSimpleGameSound(data,
				(size_t)lengths[kind], &format);
			if (sound->InitCheck() == B_OK) {
				sSfx[kind][voice].data = data;
				sSfx[kind][voice].sound = sound;
			} else {
				// VM chưa cấu hình audio: game vẫn chạy im lặng.
				delete sound;
				delete[] data;
			}
		}
	}
}

void
PlaySfx(int32 kind)
{
	if (!sSfxInited || sSfxMuted || kind < 0 || kind >= SFX_COUNT) return;
	int32 slot = sSfxCursor[kind]++ % kSfxVoices;
	if (sSfx[kind][slot].sound != NULL)
		sSfx[kind][slot].sound->StartPlaying();
}

void
SetSfxMuted(bool muted)
{
	sSfxMuted = muted;
}

void
ShutdownSfx()
{
	for (int32 kind = 0; kind < SFX_COUNT; kind++) {
		for (int32 voice = 0; voice < kSfxVoices; voice++) {
			delete sSfx[kind][voice].sound;
			delete[] sSfx[kind][voice].data;
			sSfx[kind][voice].sound = NULL;
			sSfx[kind][voice].data = NULL;
		}
	}
	sSfxInited = false;
}

} // unnamed namespace

// ---------------------------------------------------------------------------

GameView::GameView()
	: BView(BRect(0, 0, 760, 600), "game", B_FOLLOW_ALL,
		B_WILL_DRAW | B_NAVIGABLE),
	  fPlayerX(350),
	  fTargetX(350),
	  fScore(0),
	  fLives(3),
	  fWave(0),
	  fPower(1),
	  fShieldFrames(0),
	  fFanFrames(0),
	  fFireFrames(0),
	  fSpawnFrames(0),
	  fTick(0),
	  fCombo(0),
	  fBestCombo(0),
	  fComboTimer(0),
	  fWaveBanner(0),
	  fGameOverTick(0),
	  fFlash(0),
	  fShake(0),
	  fShakeX(0),
	  fShakeY(0),
	  fGameOver(false),
	  fBossActive(false),
	  fSoundOn(true),
	  fShip(NULL),
	  fEnemySmall(NULL),
	  fEnemyMedium(NULL),
	  fEnemyBig(NULL),
	  fExplosion(NULL),
	  fLaser(NULL),
	  fPowerUp(NULL),
	  fClouds(NULL)
{
	SetViewColor(3, 7, 24);
	for (int32 i = 0; i < kTrailCount; i++) fPrevX[i] = 350;
	LoadSprites();
	InitStars();
	srand((unsigned)time(NULL));
	InitSfxEngine();
	ResetGame();
}

GameView::~GameView()
{
	ShutdownSfx();
	delete fShip;
	delete fEnemySmall;
	delete fEnemyMedium;
	delete fEnemyBig;
	delete fExplosion;
	delete fLaser;
	delete fPowerUp;
	delete fClouds;
}

void
GameView::LoadSprites()
{
	app_info appInfo;
	if (be_app == NULL || be_app->GetAppInfo(&appInfo) != B_OK)
		return;

	BEntry executable(&appInfo.ref);
	BPath path(&executable);
	path.GetParent(&path);
	path.GetParent(&path);

	BPath assets(path);
	assets.Append("assets_cc0/space_shooter_pack/spritesheets");

	BPath p;
	p = assets; p.Append("ship.png");
	fShip = BTranslationUtils::GetBitmapFile(p.Path());
	p = assets; p.Append("enemy-small.png");
	fEnemySmall = BTranslationUtils::GetBitmapFile(p.Path());
	p = assets; p.Append("enemy-medium.png");
	fEnemyMedium = BTranslationUtils::GetBitmapFile(p.Path());
	p = assets; p.Append("enemy-big.png");
	fEnemyBig = BTranslationUtils::GetBitmapFile(p.Path());
	p = assets; p.Append("explosion.png");
	fExplosion = BTranslationUtils::GetBitmapFile(p.Path());
	p = assets; p.Append("laser-bolts.png");
	fLaser = BTranslationUtils::GetBitmapFile(p.Path());
	p = assets; p.Append("power-up.png");
	fPowerUp = BTranslationUtils::GetBitmapFile(p.Path());

	BPath backgrounds(path);
	backgrounds.Append("assets_cc0/space_shooter_pack/backgrounds");
	backgrounds.Append("clouds-transparent.png");
	fClouds = BTranslationUtils::GetBitmapFile(backgrounds.Path());
}

void
GameView::InitStars()
{
	for (int32 i = 0; i < kStarCount; i++) {
		Star& star = fStars[i];
		star.layer = i % 3;
		star.x = rand() % 760;
		star.y = rand() % 600;
		star.speed = 0.35f + star.layer * 1.15f + (rand() % 30) / 40.0f;
		star.size = 1 + star.layer;
		star.brightness = 100 + rand() % 155;
	}
}

void
GameView::AttachedToWindow()
{
	MakeFocus(true);
}

void
GameView::ResetGame()
{
	fPlayerX = fTargetX = 350;
	for (int32 i = 0; i < kTrailCount; i++) fPrevX[i] = 350;
	fScore = 0;
	fLives = 3;
	fWave = 0;
	fPower = 1;
	fShieldFrames = 0;
	fFanFrames = 0;
	fFireFrames = 0;
	fSpawnFrames = 0;
	fTick = 0;
	fCombo = 0;
	fBestCombo = 0;
	fComboTimer = 0;
	fWaveBanner = 0;
	fGameOverTick = 0;
	fFlash = 0;
	fShake = 0;
	fGameOver = false;
	fBossActive = false;
	fBoss.alive = false;
	for (int32 i = 0; i < kBulletCount; i++) fBullets[i].active = false;
	for (int32 i = 0; i < kItemCount; i++) fItems[i].active = false;
	for (int32 i = 0; i < kExplosionCount; i++) fExplosions[i].active = false;
	for (int32 i = 0; i < kParticleCount; i++) fParticles[i].life = 0;
	StartWave();
}

void
GameView::StartWave()
{
	fWave++;
	fWaveBanner = 120;

	bool bossWave = (fWave % 5 == 0);
	fBossActive = false;
	fBoss.alive = false;

	for (int32 i = 0; i < kEnemyCount; i++) {
		Enemy& enemy = fEnemies[i];
		int32 row = i / 7;
		int32 col = i % 7;
		enemy.baseX = 70 + col * 96;
		enemy.x = enemy.baseX;
		enemy.y = 104 + row * 66;
		enemy.phase = col * .62f + row * 1.7f;
		enemy.flash = 0;
		enemy.entry = 12 + i * 2;
		enemy.fireDelay = 60 + rand() % 180;

		if (fWave >= 3 && i % 7 == 0)
			enemy.kind = kEnemyBig;
		else if (fWave >= 2 && i % 3 == 0)
			enemy.kind = kEnemyMedium;
		else
			enemy.kind = kEnemySmall;

		int32 baseHp = enemy.kind == kEnemyBig ? 5 :
			(enemy.kind == kEnemyMedium ? 2 : 1);
		enemy.hp = baseHp + fWave / 4;
		enemy.alive = !bossWave;
	}

	if (bossWave) {
		fBossActive = true;
		fBoss.alive = true;
		fBoss.x = fBoss.baseX = 380;
		fBoss.y = -130;
		fBoss.phase = 0;
		fBoss.hp = 90 + fWave * 22;
		fBoss.maxHp = fBoss.hp;
		fBoss.flash = 0;
		fBoss.fireTimer = 90;
		fBoss.entry = 46;
		fBoss.volley = 0;
		fWaveBanner = 150;
		PlaySfx(SFX_BOSS);
		Shake(6);
	} else {
		PlaySfx(SFX_WAVE);
	}
}

void
GameView::Shake(float amount)
{
	if (amount > fShake) fShake = amount;
}

void
GameView::SpawnParticles(float x, float y, int32 count,
	int32 color, float speed, int32 size)
{
	for (int32 n = 0; n < count; n++) {
		for (int32 i = 0; i < kParticleCount; i++) {
			Particle& p = fParticles[i];
			if (p.life > 0) continue;
			float angle = ((float)rand() / RAND_MAX) * 6.28318f;
			float velocity = speed * (0.35f + (rand() % 100) / 100.0f);
			p.x = x;
			p.y = y;
			p.vx = cos(angle) * velocity;
			p.vy = sin(angle) * velocity;
			p.maxLife = 12 + rand() % 22;
			p.life = p.maxLife;
			p.size = size + rand() % 3;
			p.color = color;
			break;
		}
	}
}

void
GameView::AddBullet(float x, float y, float dx, float dy, bool enemy)
{
	for (int32 i = 0; i < kBulletCount; i++) {
		Bullet& bullet = fBullets[i];
		if (bullet.active) continue;
		bullet.x = x;
		bullet.y = y;
		bullet.dx = dx;
		bullet.dy = dy;
		bullet.enemy = enemy;
		bullet.active = true;
		return;
	}
}

void
GameView::Draw(BRect)
{
	BRect bounds = Bounds();

	SetDrawingMode(B_OP_COPY);
	SetHighColor(3, 7, 24);
	FillRect(BRect(-24, -24, bounds.right + 24, bounds.bottom + 24));

	PushState();
	SetOrigin(BPoint(fShakeX, fShakeY));
	DrawBackground(bounds);

	SetDrawingMode(B_OP_OVER);
	for (int32 i = 0; i < kEnemyCount; i++)
		if (fEnemies[i].alive) DrawEnemy(fEnemies[i], i);
	DrawBoss();
	DrawItems();
	DrawBullets();
	DrawPlayer(bounds);
	DrawExplosions();
	DrawParticles();
	PopState();

	if (fFlash > 0) {
		SetDrawingMode(B_OP_OVER);
		SetHighColor(255, 255, 255, fFlash * 3);
		FillRect(bounds);
		SetDrawingMode(B_OP_COPY);
	}

	DrawHud(bounds);
	DrawBossHealth(bounds);
	DrawWaveBanner(bounds);
	if (fGameOver) DrawGameOver(bounds);
}

void
GameView::DrawBackground(BRect b)
{
	// Nền gradient sâu thẳm (chia băng, không tốn chi phí pixel).
	for (int32 y = 70; y < b.bottom; y += 10) {
		float t = (y - 70) / (float)(b.bottom - 70);
		int32 r = (int32)(4 + t * 12);
		int32 g = (int32)(7 + t * 18);
		int32 bl = (int32)(30 + t * 48);
		SetHighColor(r, g, bl);
		FillRect(BRect(-24, y, b.right + 24, y + 11));
	}

	// Tinh vân nhiều lớp.
	SetDrawingMode(B_OP_OVER);
	SetHighColor(36, 12, 82, 110);
	FillEllipse(BRect(-190, 116, 330, 626));
	SetHighColor(70, 24, 142, 70);
	FillEllipse(BRect(-70, 190, 300, 560));
	SetHighColor(8, 62, 122, 88);
	FillEllipse(BRect(444, 104, 966, 590));
	SetHighColor(26, 148, 184, 46);
	FillEllipse(BRect(536, 214, 900, 528));
	SetDrawingMode(B_OP_COPY);

	// Mây CC0 trôi ngang làm lớp mây vũ trụ.
	if (fClouds != NULL) {
		int32 cw = 512, ch = 206;
		int32 offset = (fTick / 3) % cw;
		SetDrawingMode(B_OP_OVER);
		for (int32 x = -cw - offset; x < b.right + cw; x += cw) {
			DrawBitmap(fClouds, BRect(0, 0, 255, 102),
				BRect(x, 92, x + cw, 92 + ch));
		}
		SetDrawingMode(B_OP_COPY);
	}

	// Sao parallax có nhấp nháy và vệt đuôi.
	for (int32 i = 0; i < kStarCount; i++) {
		Star& star = fStars[i];
		int32 twinkle = (int32)(sin(fTick * 0.045f + i * 1.7f) * 22);
		int32 v = star.brightness + twinkle;
		if (v < 40) v = 40;
		if (v > 255) v = 255;
		if (star.layer == 0)
			SetHighColor(v / 2, v / 2, v);
		else if (star.layer == 1)
			SetHighColor(v / 2, v, 255);
		else
			SetHighColor(255, v, v);
		int32 streak = star.layer * 3 + 1;
		FillRect(BRect(star.x, star.y, star.x + star.size,
			star.y + star.size + streak));
	}

	// Hành tinh có vành lớn ở góc phải.
	SetDrawingMode(B_OP_OVER);
	const float px = 636, py = 168, pr = 84;
	SetHighColor(48, 128, 196, 34);
	FillEllipse(BRect(px - pr - 26, py - pr - 26,
		px + pr + 26, py + pr + 26));
	SetHighColor(126, 96, 176, 90);
	StrokeEllipse(BRect(px - pr - 48, py - 23, px + pr + 48, py + 23));
	for (int32 y = -pr; y < pr; y += 7) {
		float halfWidth = sqrtf(pr * pr - (float)y * (float)y);
		float t = (y + pr) / (2.0f * pr);
		int32 r = (int32)(44 + 42 * t + 8 * sinf(y * 0.13f));
		int32 g = (int32)(72 + 58 * t);
		int32 bl = (int32)(124 + 66 * t);
		SetHighColor(r, g, bl);
		FillRect(BRect(px - halfWidth, py + y,
			px + halfWidth, py + y + 7));
	}
	SetHighColor(5, 9, 32, 118);
	FillEllipse(BRect(px + pr * 0.28f, py - pr,
		px + pr + 2, py + pr));
	SetHighColor(150, 214, 255, 140);
	StrokeEllipse(BRect(px - pr, py - pr, px + pr, py + pr));
	SetHighColor(198, 158, 236, 150);
	StrokeLine(BPoint(px - pr - 48, py),
		BPoint(px - pr * 0.72f, py + 17));
	StrokeLine(BPoint(px + pr * 0.72f, py + 17),
		BPoint(px + pr + 48, py));

	// Mặt trăng nhỏ bên trái.
	const float mx = 104, my = 122, mr = 32;
	SetHighColor(78, 96, 140, 60);
	FillEllipse(BRect(mx - mr - 10, my - mr - 10,
		mx + mr + 10, my + mr + 10));
	SetHighColor(118, 134, 164);
	FillEllipse(BRect(mx - mr, my - mr, mx + mr, my + mr));
	SetHighColor(92, 106, 138);
	FillEllipse(BRect(mx - 18, my - 12, mx - 4, my + 2));
	FillEllipse(BRect(mx + 8, my + 6, mx + 19, my + 17));
	SetHighColor(196, 218, 244, 140);
	StrokeEllipse(BRect(mx - mr, my - mr, mx + mr, my + mr));

	// Sao băng lướt qua.
	for (int32 i = 0; i < 3; i++) {
		int32 period = 260 + i * 90;
		int32 t = (fTick + i * 97) % period;
		if (t > 46) continue;
		float sx = 140 + i * 232 - t * 3.2f;
		float sy = 100 + i * 68 + t * 2.6f;
		SetHighColor(150, 226, 255, 210 - t * 4);
		SetPenSize(2);
		StrokeLine(BPoint(sx, sy), BPoint(sx - 34, sy - 26));
		SetPenSize(1);
	}

	// Lưới phối cảnh mỏng.
	SetHighColor(12, 58, 106, 150);
	for (int32 x = 0; x < b.right; x += 64)
		StrokeLine(BPoint(b.Width() / 2, 352), BPoint(x, b.bottom));
	for (int32 y = 398; y < b.bottom - 18; y += 30)
		StrokeLine(BPoint(0, y), BPoint(b.right, y));

	// Vignette 2 bên.
	SetHighColor(2, 5, 18);
	FillRect(BRect(-24, 0, 12, b.bottom + 24));
	FillRect(BRect(b.right - 12, 0, b.right + 24, b.bottom + 24));
	SetDrawingMode(B_OP_COPY);
}

void
GameView::DrawHud(BRect b)
{
	SetHighColor(5, 12, 38);
	FillRect(BRect(0, 0, b.right, 72));
	SetHighColor(10, 24, 64);
	FillRect(BRect(0, 58, b.right, 72));
	SetHighColor(42, 200, 255);
	StrokeLine(BPoint(0, 72), BPoint(b.right, 72));
	SetHighColor(120, 60, 180);
	StrokeLine(BPoint(0, 74), BPoint(b.right, 74));

	SetFontSize(26);
	SetHighColor(30, 80, 140);
	DrawString("NOVA ASSAULT", BPoint(27, 44));
	SetHighColor(94, 236, 255);
	DrawString("NOVA ASSAULT", BPoint(24, 41));

	char hint[96];
	snprintf(hint, sizeof(hint),
		"A/D OR MOVE MOUSE: STEER    M: SOUND %s    R: RESTART",
		fSoundOn ? "ON" : "OFF");
	SetFontSize(11);
	SetHighColor(150, 210, 245);
	DrawString(hint, BPoint(26, 60));

	char stats[128];
	snprintf(stats, sizeof(stats), "SCORE %06ld   WAVE %02ld   POWER x%ld",
		(long)fScore, (long)fWave, (long)fPower);
	SetHighColor(255, 222, 84);
	SetFontSize(14);
	DrawString(stats, BPoint(b.right - 350, 36));

	if (fShip != NULL) {
		for (int32 i = 0; i < fLives && i < 5; i++)
			DrawBitmap(fShip, BRect(0, 0, 15, 23),
				BRect(b.right - 350 + i * 22, 44,
					b.right - 334 + i * 22, 68));
	}

	SetFontSize(11);
	if (fShieldFrames > 0) {
		SetHighColor(92, 255, 214);
		DrawString("SHIELD ACTIVE", BPoint(b.right - 218, 65));
	} else {
		SetHighColor(110, 130, 165);
		DrawString("SHIELD OFFLINE", BPoint(b.right - 218, 65));
	}

	if (fFanFrames > 0) {
		char fan[24];
		snprintf(fan, sizeof(fan), "FAN %02lds", (long)(fFanFrames / 50));
		SetHighColor(196, 132, 255);
		DrawString(fan, BPoint(b.right - 94, 65));
	}

	if (fCombo > 1) {
		char combo[32];
		snprintf(combo, sizeof(combo), "COMBO x%ld", (long)fCombo);
		SetFontSize(13);
		SetHighColor(255, 140, 60);
		DrawString(combo, BPoint(b.right - 110, 36));
	}
}

void
GameView::DrawWaveBanner(BRect b)
{
	if (fWaveBanner <= 0) return;
	int32 alpha = fWaveBanner < 40 ? fWaveBanner * 6 : 240;

	char text[48];
	if (fBossActive && fBoss.alive)
		snprintf(text, sizeof(text), "WARNING: BOSS");
	else
		snprintf(text, sizeof(text), "WAVE %02ld", (long)fWave);

	float width = StringWidth(text);
	SetDrawingMode(B_OP_OVER);
	SetFontSize(fBossActive && fBoss.alive ? 32 : 42);
	SetHighColor(0, 0, 0, alpha / 2);
	DrawString(text, BPoint(b.Width() / 2 - width / 2 + 2, 252));
	SetHighColor(fBossActive && fBoss.alive ? 255 : 120,
		fBossActive && fBoss.alive ? 130 : 245,
		255, alpha);
	DrawString(text, BPoint(b.Width() / 2 - width / 2, 250));
	SetDrawingMode(B_OP_COPY);
}

void
GameView::DrawBoss()
{
	if (!fBossActive || !fBoss.alive) return;
	float cx = fBoss.x;
	float cy = fBoss.y;

	SetDrawingMode(B_OP_OVER);
	int32 pulse = (int32)(8 * (0.5f + 0.5f * sin(fTick * 0.09f)));
	SetHighColor(190, 30, 70, 42);
	FillEllipse(BRect(cx - 118 - pulse, cy - 118 - pulse,
		cx + 118 + pulse, cy + 118 + pulse));
	SetHighColor(60, 150, 255, 38);
	FillEllipse(BRect(cx - 100, cy - 100, cx + 100, cy + 100));

	if (fBoss.entry > 0) {
		SetHighColor(120, 220, 255, 150);
		FillEllipse(BRect(cx - 60, cy - 60, cx + 60, cy + 60));
		SetDrawingMode(B_OP_COPY);
		return;
	}

	if (fEnemyBig != NULL) {
		int32 frame = (fTick / 8) % 2;
		DrawBitmap(fEnemyBig, BRect(frame * 32, 0, frame * 32 + 31, 31),
			BRect(cx - 128, cy - 128, cx + 128, cy + 128));
	} else {
		SetHighColor(102, 30, 60);
		FillEllipse(BRect(cx - 116, cy - 96, cx + 116, cy + 96));
	}

	int32 beat = (int32)(10 * (0.5f + 0.5f * sin(fTick * 0.16f)));
	SetHighColor(255, 60, 90, 180);
	FillEllipse(BRect(cx - 22 - beat, cy - 14 - beat,
		cx + 22 + beat, cy + 14 + beat));
	SetHighColor(255, 226, 180, 225);
	FillEllipse(BRect(cx - 10, cy - 6, cx + 10, cy + 6));

	for (int32 side = -1; side <= 1; side += 2) {
		float gx = cx + side * 74;
		SetHighColor(38, 58, 100, 235);
		FillEllipse(BRect(gx - 20, cy + 56, gx + 20, cy + 96));
		SetHighColor(255, 116, 62, 210);
		FillEllipse(BRect(gx - 9, cy + 72, gx + 9, cy + 94));
	}

	if (fBoss.flash > 0) {
		SetHighColor(255, 255, 255, 165);
		FillEllipse(BRect(cx - 118, cy - 118, cx + 118, cy + 118));
	}
	SetDrawingMode(B_OP_COPY);
}

void
GameView::DrawBossHealth(BRect b)
{
	if (!fBossActive || !fBoss.alive) return;

	float width = 420;
	float x = (b.Width() - width) / 2;
	float y = 86;
	float ratio = (float)fBoss.hp / (float)fBoss.maxHp;
	if (ratio < 0) ratio = 0;
	if (ratio > 1) ratio = 1;

	SetDrawingMode(B_OP_OVER);
	SetHighColor(4, 8, 26, 225);
	FillRect(BRect(x - 8, y - 17, x + width + 8, y + 13));
	SetHighColor(255, 80, 110, 220);
	StrokeRect(BRect(x - 8, y - 17, x + width + 8, y + 13));

	SetFontSize(12);
	SetHighColor(255, 160, 176);
	DrawString("BOSS - OVERLORD", BPoint(x, y - 3));

	SetHighColor(150, 24, 54);
	FillRect(BRect(x, y + 2, x + width, y + 9));
	SetHighColor(255, 92, 68);
	FillRect(BRect(x, y + 2, x + width * ratio, y + 9));
	SetHighColor(255, 226, 150);
	FillRect(BRect(x, y + 2, x + width * ratio, y + 4));
	SetDrawingMode(B_OP_COPY);
}

void
GameView::DrawPlayer(BRect b)
{
	float py = b.bottom - 96;
	float px = fPlayerX + 38;

	SetHighColor(255, 96, 24);
	FillTriangle(BPoint(px - 11, py + 20), BPoint(px + 11, py + 20),
		BPoint(px, py + 48 + (fTick % 8)));
	SetHighColor(255, 186, 46);
	FillTriangle(BPoint(px - 7, py + 20), BPoint(px + 7, py + 20),
		BPoint(px, py + 38 + (fTick % 6)));
	SetHighColor(255, 248, 190);
	FillTriangle(BPoint(px - 3, py + 20), BPoint(px + 3, py + 20),
		BPoint(px, py + 30 + (fTick % 4)));

	if (fabs(fTargetX - fPlayerX) > 3) {
		for (int32 i = kTrailCount - 1; i >= 1; i--) {
			int32 alpha = 110 / (i + 1);
			SetHighColor(90, 190, 255, alpha);
			BRect r(fPrevX[i] + 38 - 18, py - 26,
				fPrevX[i] + 38 + 18, py + 26);
			FillEllipse(r);
		}
	}

	if (fShieldFrames > 0) {
		int32 ring = (fTick / 4) % 4;
		SetHighColor(52, 180, 255, 150);
		StrokeEllipse(BRect(px - 33 - ring, py - 39 - ring,
			px + 33 + ring, py + 39 + ring));
		SetHighColor(150, 255, 240, 100);
		StrokeEllipse(BRect(px - 26, py - 32, px + 26, py + 32));
	}

	if (fShip != NULL) {
		int32 frame = (fTick / 5) % 5;
		BRect src(frame * 16, 0, frame * 16 + 15, 23);
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fShip, src,
			BRect(px - 19, py - 27, px + 19, py + 27));
		return;
	}

	SetHighColor(102, 45, 255);
	FillTriangle(BPoint(px, py - 27), BPoint(px - 19, py + 24),
		BPoint(px + 19, py + 24));
	SetHighColor(224, 46, 84);
	FillTriangle(BPoint(px, py - 22), BPoint(px - 13, py + 19),
		BPoint(px + 13, py + 19));
}

void
GameView::DrawEnemy(const Enemy& enemy, int32 index)
{
	float cx = enemy.x + 25;
	float cy = enemy.y + 24;

	BBitmap* sheet = NULL;
	int32 cols = 2, fw = 16, fh = 16;
	float scale = 2.0f;

	switch (enemy.kind) {
		case kEnemyMedium:
			sheet = fEnemyMedium; cols = 4; scale = 2.5f; break;
		case kEnemyBig:
			sheet = fEnemyBig; cols = 2; fw = 32; fh = 32; scale = 2.3f; break;
		default:
			sheet = fEnemySmall; cols = 2; break;
	}

	if (sheet != NULL && enemy.entry <= 0) {
		int32 frame = (fTick / 5 + index) % cols;
		BRect src(frame * fw, 0, frame * fw + fw - 1, fh - 1);
		float w = fw * scale / 2.0f;
		float h = fh * scale / 2.0f;
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(sheet, src, BRect(cx - w, cy - h, cx + w, cy + h));
	} else if (enemy.entry > 0) {
		int32 alpha = 255 - enemy.entry * 18;
		SetHighColor(120, 220, 255, alpha);
		FillEllipse(BRect(cx - 18, cy - 18, cx + 18, cy + 18));
	}

	if (enemy.flash > 0) {
		SetHighColor(255, 255, 255, 150);
		FillEllipse(BRect(cx - 24, cy - 24, cx + 24, cy + 24));
	}

	if (enemy.kind == kEnemyBig && enemy.hp > 1) {
		SetHighColor(255, 90, 120);
		FillRect(BRect(cx - 20, cy - 30, cx - 20 + enemy.hp * 5, cy - 27));
	}
}

void
GameView::DrawBullets()
{
	for (int32 i = 0; i < kBulletCount; i++) {
		Bullet& bullet = fBullets[i];
		if (!bullet.active) continue;

		// Vệt đạn nghiêng theo hướng bay (đạn quạt nhìn rõ hơn).
		if (bullet.dx != 0) {
			SetPenSize(2);
			SetHighColor(bullet.enemy ? 255 : 90,
				bullet.enemy ? 110 : 220,
				bullet.enemy ? 150 : 255, 150);
			StrokeLine(
				BPoint(bullet.x - bullet.dx * 1.2f,
					bullet.y - bullet.dy * 1.2f),
				BPoint(bullet.x, bullet.y));
			SetPenSize(1);
		}

		// Vầng sáng tròn + lõi sáng (không còn ô chữ nhật).
		if (!bullet.enemy) {
			SetHighColor(40, 150, 255, 35);
			FillEllipse(BRect(bullet.x - 12, bullet.y - 22,
				bullet.x + 12, bullet.y + 22));
			SetHighColor(90, 230, 255, 70);
			FillEllipse(BRect(bullet.x - 7, bullet.y - 17,
				bullet.x + 7, bullet.y + 17));
		} else {
			SetHighColor(255, 50, 110, 35);
			FillEllipse(BRect(bullet.x - 11, bullet.y - 20,
				bullet.x + 11, bullet.y + 20));
			SetHighColor(255, 120, 170, 70);
			FillEllipse(BRect(bullet.x - 6, bullet.y - 15,
				bullet.x + 6, bullet.y + 15));
		}

		if (fLaser != NULL && bullet.dx == 0) {
			int32 x0 = bullet.enemy ? 16 : 0;
			BRect src(x0, 16, x0 + 15, 31);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(fLaser, src, BRect(bullet.x - 6, bullet.y - 15,
				bullet.x + 6, bullet.y + 15));
		} else {
			SetHighColor(bullet.enemy ? 255 : 90,
				bullet.enemy ? 90 : 210, bullet.enemy ? 140 : 255);
			FillEllipse(BRect(bullet.x - 4, bullet.y - 10,
				bullet.x + 4, bullet.y + 10));
		}

		SetHighColor(255, 255, 255, 205);
		FillEllipse(BRect(bullet.x - 2, bullet.y - 8,
			bullet.x + 2, bullet.y + 8));
	}
}

void
GameView::DrawItems()
{
	for (int32 i = 0; i < kItemCount; i++) {
		Item& item = fItems[i];
		if (!item.active) continue;

		int32 pulse = (item.phase / 4) % 3;
		uint8 r = 255, g = 200, bl = 60;
		if (item.type == kItemShield) { r = 70; g = 240; bl = 210; }
		if (item.type == kItemFan) { r = 190; g = 120; bl = 255; }

		SetHighColor(r, g, bl, 90);
		FillEllipse(BRect(item.x - 20 - pulse, item.y - 20 - pulse,
			item.x + 20 + pulse, item.y + 20 + pulse));

		if (fPowerUp != NULL && item.type != kItemFan) {
			int32 frame = (item.type == kItemPower ? 0 : 2) +
				((item.phase / 8) % 2);
			BRect src((frame % 2) * 16, (frame / 2) * 16,
				(frame % 2) * 16 + 15, (frame / 2) * 16 + 15);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(fPowerUp, src, BRect(item.x - 17, item.y - 17,
				item.x + 17, item.y + 17));
		} else {
			SetHighColor(r, g, bl);
			FillEllipse(BRect(item.x - 15, item.y - 15,
				item.x + 15, item.y + 15));
		}

		SetDrawingMode(B_OP_COPY);
		SetHighColor(18, 22, 38);
		SetFontSize(14);
		const char* label = item.type == kItemPower ? "P" :
			(item.type == kItemShield ? "S" : "F");
		DrawString(label, BPoint(item.x - 4, item.y + 5));
	}
}

void
GameView::DrawExplosions()
{
	if (fExplosion == NULL) return;
	SetDrawingMode(B_OP_OVER);
	for (int32 i = 0; i < kExplosionCount; i++) {
		Explosion& boom = fExplosions[i];
		if (!boom.active) continue;

		BRect src(boom.frame * 16, 0, boom.frame * 16 + 15, 15);
		DrawBitmap(fExplosion, src, BRect(boom.x - boom.size / 2,
			boom.y - boom.size / 2, boom.x + boom.size / 2,
			boom.y + boom.size / 2));

		SetHighColor(255, 200, 120, 160 - boom.frame * 28);
		StrokeEllipse(BRect(boom.x - boom.size / 2 - boom.frame * 4,
			boom.y - boom.size / 2 - boom.frame * 4,
			boom.x + boom.size / 2 + boom.frame * 4,
			boom.y + boom.size / 2 + boom.frame * 4));
	}
}

void
GameView::DrawParticles()
{
	SetDrawingMode(B_OP_OVER);
	for (int32 i = 0; i < kParticleCount; i++) {
		Particle& p = fParticles[i];
		if (p.life <= 0) continue;
		int32 alpha = (int32)(255.0f * p.life / p.maxLife);
		uint8 r = 255, g = 220, bl = 140;
		switch (p.color) {
			case 0: r = 255; g = 180; bl = 60; break;
			case 1: r = 90; g = 220; bl = 255; break;
			case 2: r = 255; g = 90; bl = 140; break;
			case 3: r = 200; g = 255; bl = 170; break;
		}
		SetHighColor(r, g, bl, alpha);
		FillRect(BRect(p.x - p.size, p.y - p.size,
			p.x + p.size, p.y + p.size));
	}
}

void
GameView::DrawGameOver(BRect b)
{
	int32 t = fGameOverTick;
	int32 fade = t < 18 ? t * 14 : 250;
	if (fade > 250) fade = 250;

	SetDrawingMode(B_OP_OVER);

	SetHighColor(3, 2, 12, 190);
	FillRect(b);
	SetHighColor(105, 12, 36, 90);
	FillEllipse(BRect(-170, 120, 380, 640));
	SetHighColor(150, 20, 55, 70);
	FillEllipse(BRect(400, 100, 940, 600));

	BRect panel(168, 168, 592, 432);
	SetHighColor(9, 12, 32, 235);
	FillRect(panel);
	SetHighColor(210, 60, 95, 210);
	StrokeRect(panel);

	SetHighColor(255, 110, 140, 235);
	StrokeLine(BPoint(panel.left, panel.top + 22), BPoint(panel.left, panel.top));
	StrokeLine(BPoint(panel.left, panel.top), BPoint(panel.left + 22, panel.top));
	StrokeLine(BPoint(panel.right - 22, panel.bottom), BPoint(panel.right, panel.bottom));
	StrokeLine(BPoint(panel.right, panel.bottom), BPoint(panel.right, panel.bottom - 22));

	const char* title = "GAME OVER";
	SetFontSize(44);
	SetHighColor(120, 0, 28, fade);
	DrawString(title, BPoint(b.Width() / 2 - 128, 232));
	SetHighColor(255, 86, 112, fade);
	DrawString(title, BPoint(b.Width() / 2 - 130, 230));
	SetHighColor(255, 230, 235, fade);
	DrawString(title, BPoint(b.Width() / 2 - 132, 228));

	SetHighColor(255, 96, 126, 170);
	StrokeLine(BPoint(208, 254), BPoint(552, 254));

	char line[64];
	SetFontSize(13);
	SetHighColor(160, 195, 230, fade);
	DrawString("FINAL SCORE", BPoint(210, 286));
	DrawString("WAVE REACHED", BPoint(400, 286));

	SetFontSize(22);
	SetHighColor(255, 220, 84, fade);
	snprintf(line, sizeof(line), "%06ld", (long)fScore);
	DrawString(line, BPoint(210, 312));
	SetHighColor(110, 235, 255, fade);
	snprintf(line, sizeof(line), "%02ld", (long)fWave);
	DrawString(line, BPoint(400, 312));

	SetFontSize(13);
	SetHighColor(160, 195, 230, fade);
	DrawString("BEST COMBO", BPoint(210, 346));
	SetHighColor(255, 150, 80, fade);
	snprintf(line, sizeof(line), "x%ld", (long)fBestCombo);
	DrawString(line, BPoint(400, 346));

	int32 pulse = 120 + (int32)(120 * (0.5 + 0.5 * sin(t * 0.11)));
	SetFontSize(17);
	SetHighColor(190, 245, 255, pulse);
	DrawString("PRESS R TO RESTART", BPoint(b.Width() / 2 - 116, 398));

	BRect key(b.Width() / 2 + 92, 382, b.Width() / 2 + 116, 406);
	SetHighColor(30, 52, 82, 235);
	FillRect(key);
	SetHighColor(190, 245, 255, 235);
	StrokeRect(key);
	SetFontSize(14);
	SetHighColor(255, 255, 255, pulse);
	DrawString("R", BPoint(b.Width() / 2 + 100, 402));

	SetDrawingMode(B_OP_COPY);
}

void
GameView::KeyDown(const char* bytes, int32 count)
{
	if (count <= 0) return;
	if (bytes[0] == 'a' || bytes[0] == 'A')
		fTargetX -= 50;
	else if (bytes[0] == 'd' || bytes[0] == 'D')
		fTargetX += 50;
	else if (bytes[0] == 'm' || bytes[0] == 'M') {
		fSoundOn = !fSoundOn;
		SetSfxMuted(!fSoundOn);
		if (fSoundOn) PlaySfx(SFX_POWER);
	} else if (bytes[0] == 'r' || bytes[0] == 'R') {
		ResetGame();
		return;
	}
	if (fTargetX < 32) fTargetX = 32;
	if (fTargetX > Bounds().right - 115) fTargetX = Bounds().right - 115;
}

void
GameView::MouseDown(BPoint point)
{
	fTargetX = point.x - 38;
	if (fTargetX < 32) fTargetX = 32;
	if (fTargetX > Bounds().right - 115) fTargetX = Bounds().right - 115;
	MakeFocus(true);
}

void
GameView::MouseMoved(BPoint point, uint32 transit, const BMessage*)
{
	if (transit == B_EXITED_VIEW) return;
	fTargetX = point.x - 38;
	if (fTargetX < 32) fTargetX = 32;
	if (fTargetX > Bounds().right - 115) fTargetX = Bounds().right - 115;
}

void
GameView::MessageReceived(BMessage* message)
{
	if (message->what == MSG_FRAME)
		UpdateGame();
	else
		BView::MessageReceived(message);
}

void
GameView::UpdateParticles()
{
	for (int32 i = 0; i < kParticleCount; i++) {
		Particle& p = fParticles[i];
		if (p.life <= 0) continue;
		p.x += p.vx;
		p.y += p.vy;
		p.vy += 0.035f;
		p.vx *= 0.985f;
		p.life--;
	}
}

void
GameView::FirePlayer()
{
	float muzzleX = fPlayerX + 38;
	float muzzleY = Bounds().bottom - 134;

	if (fFanFrames > 0) {
		int32 count = fPower >= 3 ? 7 : 5;
		float spread = fPower >= 3 ? 0.62f : 0.50f;
		for (int32 n = 0; n < count; n++) {
			float angle = -spread + (2.0f * spread * n) / (count - 1);
			AddBullet(muzzleX, muzzleY,
				sinf(angle) * 13.5f, -cosf(angle) * 13.5f, false);
		}
		fFireFrames = 16;
	} else {
		for (int32 n = 0; n < fPower; n++) {
			AddBullet(muzzleX + (n - (fPower - 1) / 2.0f) * 20,
				muzzleY, 0, -13, false);
		}
		fFireFrames = fPower >= 3 ? 11 : 15;
	}

	if (fTick % 2 == 0) PlaySfx(SFX_SHOOT);
	SpawnParticles(muzzleX, Bounds().bottom - 118, 2, 1, 1.4f, 2);
}

void
GameView::FireEnemy(Enemy& enemy)
{
	float speed = enemy.kind == kEnemyBig ? 7.5f : 5.4f;
	AddBullet(enemy.x + 25, enemy.y + 44, 0, speed, true);
}

void
GameView::FireBoss()
{
	if (!fBoss.alive || fBoss.entry > 0) return;
	fBoss.volley++;

	if (fBoss.volley % 3 == 0) {
		float dx = (fPlayerX + 38) - fBoss.x;
		float dy = (Bounds().bottom - 96) - (fBoss.y + 90);
		float length = sqrtf(dx * dx + dy * dy);
		if (length < 1) length = 1;
		dx /= length;
		dy /= length;
		for (int32 i = -2; i <= 2; i++) {
			float a = i * 0.11f;
			float ca = cosf(a), sa = sinf(a);
			float rx = dx * ca - dy * sa;
			float ry = dx * sa + dy * ca;
			AddBullet(fBoss.x, fBoss.y + 90, rx * 7.2f, ry * 7.2f, true);
		}
	} else {
		for (int32 i = -3; i <= 3; i++) {
			float a = i * 0.19f;
			AddBullet(fBoss.x, fBoss.y + 90,
				sinf(a) * 6.6f, cosf(a) * 6.6f, true);
		}
	}

	PlaySfx(SFX_HIT);
	SpawnParticles(fBoss.x, fBoss.y + 96, 8, 2, 2.0f, 2);
}

void
GameView::DamageBoss(int32 amount, float x, float y)
{
	if (!fBossActive || !fBoss.alive || fBoss.entry > 0) return;

	fBoss.hp -= amount;
	fBoss.flash = 3;
	SpawnParticles(x, y, 4, 2, 2.2f, 2);
	PlaySfx(SFX_HIT);

	if (fBoss.hp > 0) return;

	fBoss.alive = false;
	fScore += 2500 + fWave * 200;
	fCombo += 3;
	if (fCombo > fBestCombo) fBestCombo = fCombo;
	fComboTimer = 220;

	for (int32 i = 0; i < 6; i++) {
		float ox = ((float)rand() / RAND_MAX - 0.5f) * 180;
		float oy = ((float)rand() / RAND_MAX - 0.5f) * 150;
		SpawnExplosion(fBoss.x + ox, fBoss.y + oy, 70 + rand() % 60);
	}
	SpawnParticles(fBoss.x, fBoss.y, 60, 0, 4.0f, 5);
	SpawnParticles(fBoss.x, fBoss.y, 30, 2, 3.0f, 4);
	SpawnItem(fBoss.x, fBoss.y, kItemFan);
	SpawnItem(fBoss.x - 60, fBoss.y + 20, kItemPower);
	SpawnItem(fBoss.x + 60, fBoss.y + 20, kItemShield);
	Shake(16);
	fFlash = 45;
	PlaySfx(SFX_BOOM);
}

void
GameView::SpawnItem(float x, float y, int32 forcedType)
{
	for (int32 i = 0; i < kItemCount; i++) {
		if (fItems[i].active) continue;
		int32 type = forcedType;
		if (type < 0) {
			int32 roll = rand() % 100;
			if (roll < 12)
				type = kItemFan;
			else if (roll < 32)
				type = kItemShield;
			else
				type = kItemPower;
		}
		fItems[i].x = x;
		fItems[i].y = y;
		fItems[i].type = type;
		fItems[i].phase = 0;
		fItems[i].active = true;
		return;
	}
}

void
GameView::SpawnExplosion(float x, float y, int32 size)
{
	for (int32 i = 0; i < kExplosionCount; i++) {
		if (fExplosions[i].active) continue;
		fExplosions[i].x = x;
		fExplosions[i].y = y;
		fExplosions[i].frame = 0;
		fExplosions[i].size = size;
		fExplosions[i].active = true;
		return;
	}
}

bool
GameView::Hits(float ax, float ay, float ar,
	float bx, float by, float br) const
{
	float dx = ax - bx, dy = ay - by;
	return dx * dx + dy * dy < (ar + br) * (ar + br);
}

void
GameView::UpdateGame()
{
	fTick++;
	BRect bounds = Bounds();

	if (fShake > 0.4f) {
		fShakeX = ((float)rand() / RAND_MAX - 0.5f) * fShake;
		fShakeY = ((float)rand() / RAND_MAX - 0.5f) * fShake;
		fShake *= 0.86f;
	} else {
		fShake = 0; fShakeX = 0; fShakeY = 0;
	}
	if (fFlash > 0) fFlash -= 4;
	if (fComboTimer > 0 && --fComboTimer == 0) fCombo = 0;
	if (fWaveBanner > 0) fWaveBanner--;
	if (fFanFrames > 0) fFanFrames--;

	for (int32 i = 0; i < kStarCount; i++) {
		Star& star = fStars[i];
		star.y += star.speed;
		if (star.y > bounds.bottom) {
			star.y = 72;
			star.x = rand() % (int32)bounds.Width();
		}
	}

	for (int32 i = kTrailCount - 1; i >= 1; i--)
		fPrevX[i] = fPrevX[i - 1];
	fPrevX[0] = fPlayerX;

	UpdateParticles();

	if (fGameOver) {
		fGameOverTick++;
		Invalidate();
		return;
	}

	if (fPlayerX < fTargetX) fPlayerX += 9;
	if (fPlayerX > fTargetX) fPlayerX -= 9;
	if (fabs(fPlayerX - fTargetX) < 9) fPlayerX = fTargetX;

	if (fFireFrames > 0) fFireFrames--;
	if (fShieldFrames > 0) fShieldFrames--;
	if (fFireFrames == 0) FirePlayer();

	int32 alive = 0;
	for (int32 i = 0; i < kEnemyCount; i++) {
		Enemy& enemy = fEnemies[i];
		if (!enemy.alive) continue;
		alive++;

		if (enemy.entry > 0) {
			enemy.entry--;
			continue;
		}

		enemy.phase += .035f;
		enemy.x = enemy.baseX + sin(enemy.phase) * 36;
		enemy.y += .028f + fWave * .005f;
		if (enemy.flash > 0) enemy.flash--;
		if (enemy.fireDelay > 0) enemy.fireDelay--;
		else {
			FireEnemy(enemy);
			enemy.fireDelay = 110 + rand() % 170 - fWave * 4;
			if (enemy.fireDelay < 55) enemy.fireDelay = 55;
		}

		if (enemy.y > bounds.bottom + 60)
			enemy.alive = false;
	}

	if (fBossActive && fBoss.alive) {
		alive++;
		if (fBoss.entry > 0) {
			fBoss.entry--;
			fBoss.y += 6;
		} else {
			fBoss.phase += 0.018f;
			fBoss.x = 380 + sin(fBoss.phase) * 205;
			fBoss.y = 168 + sin(fBoss.phase * 2.3f) * 16;
			if (fBoss.flash > 0) fBoss.flash--;
			if (--fBoss.fireTimer <= 0) {
				FireBoss();
				fBoss.fireTimer = 26 + (fWave > 10 ? 10 : 0)
					+ rand() % 14 - fWave;
				if (fBoss.fireTimer < 18) fBoss.fireTimer = 18;
			}
		}
	}

	if (alive == 0) {
		if (fSpawnFrames == 0)
			fSpawnFrames = 60;
		else if (--fSpawnFrames == 0)
			StartWave();
	} else {
		fSpawnFrames = 0;
	}

	for (int32 i = 0; i < kBulletCount; i++) {
		Bullet& bullet = fBullets[i];
		if (!bullet.active) continue;

		bullet.x += bullet.dx;
		bullet.y += bullet.dy;
		if (bullet.x < -30 || bullet.x > bounds.right + 30 ||
			bullet.y < 74 || bullet.y > bounds.bottom) {
			bullet.active = false;
			continue;
		}

		if (!bullet.enemy && fTick % 3 == 0)
			SpawnParticles(bullet.x, bullet.y + 12, 1, 1, 0.4f, 1);

		if (!bullet.enemy) {
			bool consumed = false;
			for (int32 e = 0; e < kEnemyCount; e++) {
				Enemy& enemy = fEnemies[e];
				if (!enemy.alive || enemy.entry > 0) continue;
				float radius = enemy.kind == kEnemyBig ? 34 :
					(enemy.kind == kEnemyMedium ? 22 : 16);
				if (Hits(bullet.x, bullet.y, 6, enemy.x + 25,
					enemy.y + 24, radius)) {
					bullet.active = false;
					consumed = true;
					enemy.flash = 3;
					SpawnParticles(bullet.x, bullet.y, 3, 1, 1.6f, 2);
					if (--enemy.hp <= 0) {
						enemy.alive = false;
						int32 gain = enemy.kind == kEnemyBig ? 300 :
							(enemy.kind == kEnemyMedium ? 200 : 100);
						fCombo++;
						if (fCombo > fBestCombo) fBestCombo = fCombo;
						fComboTimer = 150;
						fScore += gain + (fCombo - 1) * 20;
						SpawnExplosion(enemy.x + 25, enemy.y + 24,
							enemy.kind == kEnemyBig ? 78 : 52);
						SpawnParticles(enemy.x + 25, enemy.y + 24,
							enemy.kind == kEnemyBig ? 18 : 10, 0, 2.6f, 3);
						Shake(enemy.kind == kEnemyBig ? 6 : 2.5f);
						PlaySfx(SFX_BOOM);
						if (rand() % 4 == 0)
							SpawnItem(enemy.x + 25, enemy.y + 24);
					} else {
						PlaySfx(SFX_HIT);
					}
					break;
				}
			}
			if (!consumed && fBossActive && fBoss.alive
				&& fBoss.entry <= 0
				&& Hits(bullet.x, bullet.y, 6, fBoss.x, fBoss.y, 86)) {
				bullet.active = false;
				DamageBoss(1, bullet.x, bullet.y);
			}
		} else if (Hits(bullet.x, bullet.y, 7, fPlayerX + 38,
			bounds.bottom - 96, 28)) {
			bullet.active = false;
			SpawnParticles(bullet.x, bullet.y, 6, 2, 2.0f, 2);
			if (fShieldFrames == 0) {
				SpawnExplosion(fPlayerX + 38, bounds.bottom - 96, 70);
				SpawnParticles(fPlayerX + 38, bounds.bottom - 96,
					16, 2, 3.0f, 3);
				Shake(9);
				fFlash = 26;
				fCombo = 0;
				PlaySfx(SFX_HURT);
				if (--fLives <= 0) {
					fGameOver = true;
					fGameOverTick = 0;
					SpawnParticles(fPlayerX + 38,
						bounds.bottom - 96, 30, 2, 3.5f, 4);
					Shake(12);
					fFlash = 40;
					PlaySfx(SFX_GAMEOVER);
				} else {
					fShieldFrames = 100;
				}
			} else {
				SpawnParticles(bullet.x, bullet.y, 8, 3, 2.2f, 2);
			}
		}
	}

	for (int32 i = 0; i < kItemCount; i++) {
		Item& item = fItems[i];
		if (!item.active) continue;

		item.phase++;
		item.y += 1.6f;
		if (item.y > bounds.bottom) {
			item.active = false;
			continue;
		}
		if (Hits(item.x, item.y, 17, fPlayerX + 38,
			bounds.bottom - 96, 30)) {
			if (item.type == kItemPower && fPower < 3) fPower++;
			if (item.type == kItemShield) fShieldFrames = 240;
			if (item.type == kItemFan) fFanFrames = 600;
			fScore += 50;
			SpawnParticles(item.x, item.y, 14, 3, 2.4f, 3);
			PlaySfx(SFX_POWER);
			item.active = false;
		}
	}

	for (int32 i = 0; i < kExplosionCount; i++) {
		Explosion& boom = fExplosions[i];
		if (!boom.active) continue;
		if (++boom.frame >= 5) boom.active = false;
	}

	Invalidate();
}
