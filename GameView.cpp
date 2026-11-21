#include "GameView.h"

#include <Application.h>
#include <Bitmap.h>
#include <Entry.h>
#include <Message.h>
#include <Path.h>
#include <Roster.h>
#include <TranslationUtils.h>
#include <View.h>

#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <time.h>

enum { MSG_FRAME = 'fram' };

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
	  fFireFrames(0),
	  fSpawnFrames(0),
	  fTick(0),
	  fCombo(0),
	  fComboTimer(0),
	  fWaveBanner(0),
	  fFlash(0),
	  fShake(0),
	  fShakeX(0),
	  fShakeY(0),
	  fGameOver(false),
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
	ResetGame();
}

GameView::~GameView()
{
	delete fShip;
	delete fEnemySmall;
	delete fEnemyMedium;
	delete fEnemyBig;
	delete fExplosion;
	delete fLaser;
	delete fPowerUp;
	delete fClouds;
}

void GameView::LoadSprites()
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

void GameView::InitStars()
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

void GameView::AttachedToWindow()
{
	MakeFocus(true);
}

void GameView::ResetGame()
{
	fPlayerX = fTargetX = 350;
	for (int32 i = 0; i < kTrailCount; i++) fPrevX[i] = 350;
	fScore = 0;
	fLives = 3;
	fWave = 0;
	fPower = 1;
	fShieldFrames = 0;
	fFireFrames = 0;
	fSpawnFrames = 0;
	fTick = 0;
	fCombo = 0;
	fComboTimer = 0;
	fWaveBanner = 0;
	fFlash = 0;
	fShake = 0;
	fGameOver = false;
	for (int32 i = 0; i < kBulletCount; i++) fBullets[i].active = false;
	for (int32 i = 0; i < kItemCount; i++) fItems[i].active = false;
	for (int32 i = 0; i < kExplosionCount; i++) fExplosions[i].active = false;
	for (int32 i = 0; i < kParticleCount; i++) fParticles[i].life = 0;
	StartWave();
}

void GameView::StartWave()
{
	fWave++;
	fWaveBanner = 110;
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
		enemy.alive = true;
	}
}

void GameView::Shake(float amount)
{
	if (amount > fShake) fShake = amount;
}

void GameView::SpawnParticles(float x, float y, int32 count,
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

void GameView::Draw(BRect)
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
	DrawWaveBanner(bounds);
	if (fGameOver) DrawGameOver(bounds);
}

void GameView::DrawBackground(BRect b)
{
	// Deep-space vertical gradient (banded, no per-pixel cost).
	for (int32 y = 70; y < b.bottom; y += 10) {
		float t = (y - 70) / (float)(b.bottom - 70);
		int32 r = (int32)(5 + t * 14);
		int32 g = (int32)(9 + t * 16);
		int32 bl = (int32)(34 + t * 40);
		SetHighColor(r, g, bl);
		FillRect(BRect(-24, y, b.right + 24, y + 11));
	}

	// Drifting CC0 cloud layer used as nebula.
	if (fClouds != NULL) {
		int32 cw = 512, ch = 206;
		int32 offset = (fTick / 3) % cw;
		SetDrawingMode(B_OP_OVER);
		for (int32 x = -cw - offset; x < b.right + cw; x += cw) {
			DrawBitmap(fClouds, BRect(0, 0, 255, 102),
				BRect(x, 96, x + cw, 96 + ch));
		}
		SetDrawingMode(B_OP_COPY);
	}

	// Colored nebula glows.
	SetHighColor(20, 12, 62);
	FillEllipse(BRect(-150, 130, 320, 610));
	SetHighColor(48, 22, 118);
	StrokeEllipse(BRect(-150, 130, 320, 610));
	SetHighColor(8, 40, 92);
	FillEllipse(BRect(490, 110, 950, 580));
	SetHighColor(58, 176, 220);
	StrokeEllipse(BRect(490, 110, 950, 580));

	// Parallax stars with motion streaks.
	for (int32 i = 0; i < kStarCount; i++) {
		Star& star = fStars[i];
		int32 v = star.brightness;
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

	// Perspective floor grid.
	SetHighColor(14, 66, 118);
	for (int32 x = 0; x < b.right; x += 64)
		StrokeLine(BPoint(b.Width() / 2, 352), BPoint(x, b.bottom));
	for (int32 y = 398; y < b.bottom - 18; y += 30)
		StrokeLine(BPoint(0, y), BPoint(b.right, y));

	// Side vignette.
	SetHighColor(2, 5, 18);
	FillRect(BRect(-24, 0, 12, b.bottom + 24));
	FillRect(BRect(b.right - 12, 0, b.right + 24, b.bottom + 24));
}

void GameView::DrawHud(BRect b)
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

	SetFontSize(11);
	SetHighColor(150, 210, 245);
	DrawString("A/D OR MOVE MOUSE: STEER    AUTO-FIRE ONLINE    R: RESTART",
		BPoint(26, 60));

	char stats[128];
	snprintf(stats, sizeof(stats), "SCORE %06ld   WAVE %02ld   POWER x%ld",
		(long)fScore, (long)fWave, (long)fPower);
	SetHighColor(255, 222, 84);
	SetFontSize(14);
	DrawString(stats, BPoint(b.right - 350, 36));

	// Lives as tiny ship icons.
	if (fShip != NULL) {
		for (int32 i = 0; i < fLives && i < 5; i++)
			DrawBitmap(fShip, BRect(0, 0, 15, 23),
				BRect(b.right - 350 + i * 22, 44,
					b.right - 334 + i * 22, 68));
	}

	if (fShieldFrames > 0) {
		SetHighColor(92, 255, 214);
		SetFontSize(11);
		DrawString("SHIELD ACTIVE", BPoint(b.right - 220, 65));
	} else {
		SetHighColor(110, 130, 165);
		SetFontSize(11);
		DrawString("SHIELD OFFLINE", BPoint(b.right - 220, 65));
	}

	if (fCombo > 1) {
		char combo[32];
		snprintf(combo, sizeof(combo), "COMBO x%ld", (long)fCombo);
		SetFontSize(13);
		SetHighColor(255, 140, 60);
		DrawString(combo, BPoint(b.right - 110, 36));
	}
}

void GameView::DrawWaveBanner(BRect b)
{
	if (fWaveBanner <= 0) return;
	int32 alpha = fWaveBanner < 40 ? fWaveBanner * 6 : 240;
	char text[32];
	snprintf(text, sizeof(text), "WAVE %02ld", (long)fWave);

	SetDrawingMode(B_OP_OVER);
	SetFontSize(42);
	SetHighColor(0, 0, 0, alpha / 2);
	DrawString(text, BPoint(b.Width() / 2 - 78, 250));
	SetHighColor(120, 245, 255, alpha);
	DrawString(text, BPoint(b.Width() / 2 - 80, 248));
	SetDrawingMode(B_OP_COPY);
}

void GameView::DrawPlayer(BRect b)
{
	float py = b.bottom - 96;
	float px = fPlayerX + 38;

	// Engine exhaust: layered cones + sparks.
	SetHighColor(255, 96, 24);
	FillTriangle(BPoint(px - 11, py + 20), BPoint(px + 11, py + 20),
		BPoint(px, py + 48 + (fTick % 8)));
	SetHighColor(255, 186, 46);
	FillTriangle(BPoint(px - 7, py + 20), BPoint(px + 7, py + 20),
		BPoint(px, py + 38 + (fTick % 6)));
	SetHighColor(255, 248, 190);
	FillTriangle(BPoint(px - 3, py + 20), BPoint(px + 3, py + 20),
		BPoint(px, py + 30 + (fTick % 4)));

	// Afterimages behind fast movement.
	if (fabs(fTargetX - fPlayerX) > 3) {
		for (int32 i = kTrailCount - 1; i >= 1; i--) {
			int32 alpha = 110 / (i + 1);
			SetHighColor(90, 190, 255, alpha);
			BRect r(fPrevX[i] + 38 - 18, py - 26,
				fPrevX[i] + 38 + 18, py + 26);
			FillEllipse(r);
		}
	}

	// Shield bubble with pulsing ring.
	if (fShieldFrames > 0) {
		int32 pulse = (fTick / 4) % 4;
		SetHighColor(52, 180, 255, 150);
		StrokeEllipse(BRect(px - 33 - pulse, py - 39 - pulse,
			px + 33 + pulse, py + 39 + pulse));
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

void GameView::DrawEnemy(const Enemy& enemy, int32 index)
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

void GameView::DrawBullets()
{
	for (int32 i = 0; i < kBulletCount; i++) {
		Bullet& bullet = fBullets[i];
		if (!bullet.active) continue;

		// Vầng sáng tròn (không còn ô chữ nhật) + lõi sáng rõ nét.
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

		if (fLaser != NULL) {
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

		// Lõi trắng sáng để nhìn rõ trên mọi nền.
		SetHighColor(255, 255, 255, 205);
		FillEllipse(BRect(bullet.x - 2, bullet.y - 8,
			bullet.x + 2, bullet.y + 8));
	}
}

void GameView::DrawItems()
{
	for (int32 i = 0; i < kItemCount; i++) {
		Item& item = fItems[i];
		if (!item.active) continue;

		int32 pulse = (item.phase / 4) % 3;
		SetHighColor(item.type == kItemPower ? 255 : 70,
			item.type == kItemPower ? 200 : 240,
			item.type == kItemPower ? 60 : 210, 90);
		FillEllipse(BRect(item.x - 20 - pulse, item.y - 20 - pulse,
			item.x + 20 + pulse, item.y + 20 + pulse));

		if (fPowerUp != NULL) {
			int32 frame = (item.type == kItemPower ? 0 : 2) +
				((item.phase / 8) % 2);
			BRect src((frame % 2) * 16, (frame / 2) * 16,
				(frame % 2) * 16 + 15, (frame / 2) * 16 + 15);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(fPowerUp, src, BRect(item.x - 17, item.y - 17,
				item.x + 17, item.y + 17));
		} else {
			SetHighColor(item.type == kItemPower ? 255 : 70,
				item.type == kItemPower ? 200 : 240,
				item.type == kItemPower ? 60 : 210);
			FillEllipse(BRect(item.x - 15, item.y - 15,
				item.x + 15, item.y + 15));
		}

		SetDrawingMode(B_OP_COPY);
		SetHighColor(18, 22, 38);
		SetFontSize(14);
		DrawString(item.type == kItemPower ? "P" : "S",
			BPoint(item.x - 4, item.y + 5));
	}
}

void GameView::DrawExplosions()
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

void GameView::DrawParticles()
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

void GameView::DrawGameOver(BRect b)
{
	SetHighColor(8, 3, 24);
	FillRect(BRect(0, 74, b.right, b.bottom));

	SetHighColor(126, 18, 52);
	FillEllipse(BRect(-130, 118, 350, 600));
	SetHighColor(255, 78, 62);
	StrokeEllipse(BRect(-130, 118, 350, 600));

	SetFontSize(40);
	SetHighColor(0, 0, 0);
	DrawString("GAME OVER", BPoint(b.Width() / 2 - 130, 272));
	SetHighColor(255, 92, 118);
	DrawString("GAME OVER", BPoint(b.Width() / 2 - 132, 270));

	SetFontSize(15);
	SetHighColor(230, 241, 255);
	DrawString("Press R to restart the assault",
		BPoint(b.Width() / 2 - 118, 304));
}

void GameView::KeyDown(const char* bytes, int32 count)
{
	if (count <= 0) return;
	if (bytes[0] == 'a' || bytes[0] == 'A')
		fTargetX -= 50;
	else if (bytes[0] == 'd' || bytes[0] == 'D')
		fTargetX += 50;
	else if (bytes[0] == 'r' || bytes[0] == 'R') {
		ResetGame();
		return;
	}
	if (fTargetX < 32) fTargetX = 32;
	if (fTargetX > Bounds().right - 115) fTargetX = Bounds().right - 115;
}

void GameView::MouseDown(BPoint point)
{
	fTargetX = point.x - 38;
	if (fTargetX < 32) fTargetX = 32;
	if (fTargetX > Bounds().right - 115) fTargetX = Bounds().right - 115;
	MakeFocus(true);
}

void GameView::MouseMoved(BPoint point, uint32 transit, const BMessage*)
{
	if (transit == B_EXITED_VIEW) return;
	fTargetX = point.x - 38;
	if (fTargetX < 32) fTargetX = 32;
	if (fTargetX > Bounds().right - 115) fTargetX = Bounds().right - 115;
}

void GameView::MessageReceived(BMessage* message)
{
	if (message->what == MSG_FRAME)
		UpdateGame();
	else
		BView::MessageReceived(message);
}

void GameView::UpdateParticles()
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

void GameView::UpdateGame()
{
	fTick++;
	BRect bounds = Bounds();

	// Screen shake + hit flash decay.
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

		bullet.y += bullet.dy;
		if (bullet.y < 74 || bullet.y > bounds.bottom) {
			bullet.active = false;
			continue;
		}

		if (!bullet.enemy && fTick % 3 == 0)
			SpawnParticles(bullet.x, bullet.y + 12, 1, 1, 0.4f, 1);

		if (!bullet.enemy) {
			for (int32 e = 0; e < kEnemyCount; e++) {
				Enemy& enemy = fEnemies[e];
				if (!enemy.alive || enemy.entry > 0) continue;
				float radius = enemy.kind == kEnemyBig ? 34 :
					(enemy.kind == kEnemyMedium ? 22 : 16);
				if (Hits(bullet.x, bullet.y, 6, enemy.x + 25,
					enemy.y + 24, radius)) {
					bullet.active = false;
					enemy.flash = 3;
					SpawnParticles(bullet.x, bullet.y, 3, 1, 1.6f, 2);
					if (--enemy.hp <= 0) {
						enemy.alive = false;
						int32 gain = enemy.kind == kEnemyBig ? 300 :
							(enemy.kind == kEnemyMedium ? 200 : 100);
						fCombo++;
						fComboTimer = 150;
						fScore += gain + (fCombo - 1) * 20;
						SpawnExplosion(enemy.x + 25, enemy.y + 24,
							enemy.kind == kEnemyBig ? 78 : 52);
						SpawnParticles(enemy.x + 25, enemy.y + 24,
							enemy.kind == kEnemyBig ? 18 : 10, 0, 2.6f, 3);
						Shake(enemy.kind == kEnemyBig ? 6 : 2.5f);
						if (rand() % 4 == 0)
							SpawnItem(enemy.x + 25, enemy.y + 24);
					}
					break;
				}
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
				if (--fLives <= 0)
					fGameOver = true;
				else
					fShieldFrames = 100;
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
			fScore += 50;
			SpawnParticles(item.x, item.y, 14, 3, 2.4f, 3);
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

void GameView::FirePlayer()
{
	for (int32 n = 0; n < fPower; n++) {
		for (int32 i = 0; i < kBulletCount; i++) {
			if (fBullets[i].active) continue;
			fBullets[i].x = fPlayerX + 38 +
				(n - (fPower - 1) / 2.0f) * 20;
			fBullets[i].y = Bounds().bottom - 134;
			fBullets[i].dy = -13;
			fBullets[i].enemy = false;
			fBullets[i].active = true;
			break;
		}
	}
	SpawnParticles(fPlayerX + 38, Bounds().bottom - 118, 2, 1, 1.4f, 2);
	fFireFrames = fPower >= 3 ? 11 : 15;
}

void GameView::FireEnemy(Enemy& enemy)
{
	for (int32 i = 0; i < kBulletCount; i++) {
		if (fBullets[i].active) continue;
		fBullets[i].x = enemy.x + 25;
		fBullets[i].y = enemy.y + 44;
		fBullets[i].dy = enemy.kind == kEnemyBig ? 7.5f : 5.4f;
		fBullets[i].enemy = true;
		fBullets[i].active = true;
		break;
	}
}

void GameView::SpawnItem(float x, float y)
{
	for (int32 i = 0; i < kItemCount; i++) {
		if (fItems[i].active) continue;
		fItems[i].x = x;
		fItems[i].y = y;
		fItems[i].type = rand() % 2;
		fItems[i].phase = 0;
		fItems[i].active = true;
		break;
	}
}

void GameView::SpawnExplosion(float x, float y, int32 size)
{
	for (int32 i = 0; i < kExplosionCount; i++) {
		if (fExplosions[i].active) continue;
		fExplosions[i].x = x;
		fExplosions[i].y = y;
		fExplosions[i].frame = 0;
		fExplosions[i].size = size;
		fExplosions[i].active = true;
		break;
	}
}

bool GameView::Hits(float ax, float ay, float ar,
	float bx, float by, float br) const
{
	float dx = ax - bx, dy = ay - by;
	return dx * dx + dy * dy < (ar + br) * (ar + br);
}
