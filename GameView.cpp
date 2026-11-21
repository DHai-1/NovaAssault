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
	  fGameOver(false),
	  fShip(NULL),
	  fEnemySmall(NULL),
	  fEnemyMedium(NULL),
	  fEnemyBig(NULL),
	  fExplosion(NULL),
	  fLaser(NULL),
	  fPowerUp(NULL)
{
	SetViewColor(3, 7, 24);
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
}

void GameView::LoadSprites()
{
	// Resolve assets relative to the executable: objects.* folder -> project folder.
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
}

void GameView::InitStars()
{
	for (int32 i = 0; i < kStarCount; i++) {
		Star& star = fStars[i];
		star.x = rand() % 760;
		star.y = rand() % 600;
		star.speed = 0.4f + (rand() % 100) / 60.0f;
		star.size = 1 + rand() % 2;
		star.brightness = 110 + rand() % 145;
	}
}

void GameView::AttachedToWindow()
{
	MakeFocus(true);
}

void GameView::ResetGame()
{
	fPlayerX = fTargetX = 350;
	fScore = 0;
	fLives = 3;
	fWave = 0;
	fPower = 1;
	fShieldFrames = 0;
	fFireFrames = 0;
	fSpawnFrames = 0;
	fTick = 0;
	fGameOver = false;
	for (int32 i = 0; i < kBulletCount; i++) fBullets[i].active = false;
	for (int32 i = 0; i < kItemCount; i++) fItems[i].active = false;
	for (int32 i = 0; i < kExplosionCount; i++) fExplosions[i].active = false;
	StartWave();
}

void GameView::StartWave()
{
	fWave++;
	for (int32 i = 0; i < kEnemyCount; i++) {
		Enemy& enemy = fEnemies[i];
		int32 row = i / 7;
		int32 col = i % 7;
		enemy.baseX = 70 + col * 96;
		enemy.x = enemy.baseX;
		enemy.y = 104 + row * 66;
		enemy.phase = col * .62f + row * 1.7f;
		enemy.flash = 0;

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

void GameView::Draw(BRect)
{
	BRect bounds = Bounds();

	SetDrawingMode(B_OP_COPY);
	SetHighColor(3, 7, 24);
	FillRect(bounds);

	DrawBackground(bounds);

	SetDrawingMode(B_OP_OVER);
	for (int32 i = 0; i < kEnemyCount; i++)
		if (fEnemies[i].alive) DrawEnemy(fEnemies[i], i);
	DrawItems();
	DrawBullets();
	DrawPlayer(bounds);
	DrawExplosions();
	SetDrawingMode(B_OP_COPY);

	DrawHud(bounds);
	if (fGameOver) DrawGameOver(bounds);
}

void GameView::DrawBackground(BRect b)
{
	// Nebula "planets" behind the starfield.
	SetHighColor(20, 12, 62);
	FillEllipse(BRect(-150, 130, 320, 610));
	SetHighColor(48, 22, 118);
	StrokeEllipse(BRect(-150, 130, 320, 610));
	SetHighColor(8, 40, 92);
	FillEllipse(BRect(490, 110, 950, 580));
	SetHighColor(58, 176, 220);
	StrokeEllipse(BRect(490, 110, 950, 580));

	// Parallax starfield with short motion streaks.
	for (int32 i = 0; i < kStarCount; i++) {
		Star& star = fStars[i];
		int32 v = star.brightness;
		SetHighColor(v > 220 ? 255 : v,
			v > 180 ? 250 : v,
			255);
		FillRect(BRect(star.x, star.y, star.x + star.size,
			star.y + star.size + (int32)(star.speed * 2)));
	}

	// Perspective floor grid.
	SetHighColor(14, 66, 118);
	for (int32 x = 0; x < b.right; x += 64)
		StrokeLine(BPoint(b.Width() / 2, 352), BPoint(x, b.bottom));
	for (int32 y = 398; y < b.bottom - 18; y += 30)
		StrokeLine(BPoint(0, y), BPoint(b.right, y));
}

void GameView::DrawHud(BRect b)
{
	SetHighColor(6, 14, 44);
	FillRect(BRect(0, 0, b.right, 70));
	SetHighColor(38, 196, 255);
	StrokeLine(BPoint(0, 70), BPoint(b.right, 70));

	SetFontSize(26);
	SetHighColor(0, 0, 0);
	DrawString("NOVA ASSAULT", BPoint(26, 43));
	SetHighColor(94, 236, 255);
	DrawString("NOVA ASSAULT", BPoint(24, 41));

	SetFontSize(11);
	SetHighColor(150, 210, 245);
	DrawString("A/D OR CLICK: STEER    AUTO-FIRE ONLINE    R: RESTART",
		BPoint(26, 60));

	char stats[128];
	snprintf(stats, sizeof(stats), "SCORE %06ld   LIVES %ld   WAVE %ld   POWER x%ld",
		(long)fScore, (long)fLives, (long)fWave, (long)fPower);
	SetHighColor(255, 222, 84);
	SetFontSize(14);
	DrawString(stats, BPoint(b.right - 340, 40));

	if (fShieldFrames > 0)
		SetHighColor(92, 255, 214);
	else
		SetHighColor(120, 140, 170);
	SetFontSize(12);
	DrawString(fShieldFrames > 0 ? "SHIELD ACTIVE" : "SHIELD OFFLINE",
		BPoint(b.right - 340, 60));
}

void GameView::DrawPlayer(BRect b)
{
	float px = fPlayerX + 38;
	float py = b.bottom - 96;

	if (fShieldFrames > 0) {
		SetHighColor(52, 180, 255, 180);
		StrokeEllipse(BRect(px - 31, py - 37, px + 31, py + 37));
		SetHighColor(150, 255, 240, 120);
		StrokeEllipse(BRect(px - 25, py - 31, px + 25, py + 31));
	}

	// Engine glow / exhaust.
	SetHighColor(255, 168, 42);
	FillTriangle(BPoint(px - 8, py + 22), BPoint(px + 8, py + 22),
		BPoint(px, py + 38 + (fTick % 6)));
	SetHighColor(255, 236, 150);
	FillTriangle(BPoint(px - 4, py + 22), BPoint(px + 4, py + 22),
		BPoint(px, py + 31 + (fTick % 4)));

	if (fShip != NULL) {
		int32 frame = (fTick / 7) % 5;
		BRect src(frame * 16, 0, frame * 16 + 15, 23);
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fShip, src, BRect(px - 18, py - 26, px + 18, py + 26));
		return;
	}

	// Vector fallback if sprites are missing.
	SetHighColor(102, 45, 255);
	FillTriangle(BPoint(px, py - 26), BPoint(px - 18, py + 22), BPoint(px + 18, py + 22));
	SetHighColor(224, 46, 84);
	FillTriangle(BPoint(px, py - 22), BPoint(px - 13, py + 18), BPoint(px + 13, py + 18));
	SetHighColor(247, 244, 255);
	FillTriangle(BPoint(px, py - 16), BPoint(px - 6, py + 14), BPoint(px + 6, py + 14));
}

void GameView::DrawEnemy(const Enemy& enemy, int32 index)
{
	float cx = enemy.x + 25;
	float cy = enemy.y + 24;

	BBitmap* sheet = NULL;
	int32 cols = 2;
	int32 fw = 16;
	int32 fh = 16;
	float scale = 2.0f;

	switch (enemy.kind) {
		case kEnemyMedium:
			sheet = fEnemyMedium;
			cols = 4;
			scale = 2.4f;
			break;
		case kEnemyBig:
			sheet = fEnemyBig;
			cols = 2;
			fw = 32;
			fh = 32;
			scale = 2.2f;
			break;
		default:
			sheet = fEnemySmall;
			cols = 2;
			break;
	}

	if (sheet != NULL) {
		int32 frame = (fTick / 6 + index) % cols;
		BRect src(frame * fw, 0, frame * fw + fw - 1, fh - 1);
		float w = fw * scale / 2.0f;
		float h = fh * scale / 2.0f;
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(sheet, src, BRect(cx - w, cy - h, cx + w, cy + h));
	} else {
		SetHighColor(228, 64, 176);
		FillTriangle(BPoint(cx, cy - 20), BPoint(cx - 18, cy + 18),
			BPoint(cx + 18, cy + 18));
		SetHighColor(97, 235, 255);
		FillEllipse(BRect(cx - 7, cy - 6, cx + 7, cy + 8));
	}

	if (enemy.flash > 0) {
		SetHighColor(255, 255, 255, 120);
		FillEllipse(BRect(cx - 22, cy - 22, cx + 22, cy + 22));
	}
}

void GameView::DrawBullets()
{
	for (int32 i = 0; i < kBulletCount; i++) {
		Bullet& bullet = fBullets[i];
		if (!bullet.active) continue;

		if (fLaser != NULL) {
			int32 x0 = bullet.enemy ? 16 : 0;
			BRect src(x0, 16, x0 + 15, 31);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(fLaser, src, BRect(bullet.x - 6, bullet.y - 14,
				bullet.x + 6, bullet.y + 14));
		} else {
			if (bullet.enemy)
				SetHighColor(255, 81, 125);
			else
				SetHighColor(67, 183, 255);
			FillEllipse(BRect(bullet.x - 4, bullet.y - 9,
				bullet.x + 4, bullet.y + 9));
		}
	}
}

void GameView::DrawItems()
{
	for (int32 i = 0; i < kItemCount; i++) {
		Item& item = fItems[i];
		if (!item.active) continue;

		if (fPowerUp != NULL) {
			int32 frame = item.type == kItemPower ? 0 : 2;
			BRect src((frame % 2) * 16, (frame / 2) * 16,
				(frame % 2) * 16 + 15, (frame / 2) * 16 + 15);
			SetDrawingMode(B_OP_OVER);
			DrawBitmap(fPowerUp, src, BRect(item.x - 16, item.y - 16,
				item.x + 16, item.y + 16));
		} else {
			if (item.type == kItemPower)
				SetHighColor(255, 200, 59);
			else
				SetHighColor(54, 243, 205);
			FillEllipse(BRect(item.x - 15, item.y - 15,
				item.x + 15, item.y + 15));
		}

		SetDrawingMode(B_OP_COPY);
		SetHighColor(20, 24, 40);
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
	}
}

void GameView::DrawGameOver(BRect b)
{
	SetHighColor(8, 3, 24);
	FillRect(BRect(0, 72, b.right, b.bottom));

	SetHighColor(126, 18, 52);
	FillEllipse(BRect(-130, 118, 350, 600));
	SetHighColor(255, 78, 62);
	StrokeEllipse(BRect(-130, 118, 350, 600));

	SetFontSize(36);
	SetHighColor(0, 0, 0);
	DrawString("GAME OVER", BPoint(b.Width() / 2 - 122, 268));
	SetHighColor(255, 92, 118);
	DrawString("GAME OVER", BPoint(b.Width() / 2 - 124, 266));

	SetFontSize(15);
	SetHighColor(230, 241, 255);
	DrawString("Press R to restart the assault",
		BPoint(b.Width() / 2 - 118, 300));
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

void GameView::MessageReceived(BMessage* message)
{
	if (message->what == MSG_FRAME)
		UpdateGame();
	else
		BView::MessageReceived(message);
}

void GameView::UpdateGame()
{
	fTick++;
	BRect bounds = Bounds();

	for (int32 i = 0; i < kStarCount; i++) {
		Star& star = fStars[i];
		star.y += star.speed;
		if (star.y > bounds.bottom) {
			star.y = 72;
			star.x = rand() % (int32)bounds.Width();
		}
	}

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

		enemy.phase += .035f;
		enemy.x = enemy.baseX + sin(enemy.phase) * 34;
		enemy.y += .028f + fWave * .005f;
		if (enemy.flash > 0) enemy.flash--;

		// Enemies that fly past the bottom are gone (this also lets the
		// next wave start even if the player misses a few).
		if (enemy.y > bounds.bottom + 60)
			enemy.alive = false;

		if (rand() % 260 == 0) FireEnemy(enemy);
	}

	if (alive == 0) {
		if (fSpawnFrames == 0)
			fSpawnFrames = 55;
		else if (--fSpawnFrames == 0)
			StartWave();
	} else {
		fSpawnFrames = 0;
	}

	for (int32 i = 0; i < kBulletCount; i++) {
		Bullet& bullet = fBullets[i];
		if (!bullet.active) continue;

		bullet.y += bullet.dy;
		if (bullet.y < 72 || bullet.y > bounds.bottom) {
			bullet.active = false;
			continue;
		}

		if (!bullet.enemy) {
			for (int32 e = 0; e < kEnemyCount; e++) {
				Enemy& enemy = fEnemies[e];
				if (!enemy.alive) continue;
				float radius = enemy.kind == kEnemyBig ? 34 :
					(enemy.kind == kEnemyMedium ? 22 : 16);
				if (Hits(bullet.x, bullet.y, 6, enemy.x + 25,
					enemy.y + 24, radius)) {
					bullet.active = false;
					enemy.flash = 3;
					if (--enemy.hp <= 0) {
						enemy.alive = false;
						int32 gain = enemy.kind == kEnemyBig ? 300 :
							(enemy.kind == kEnemyMedium ? 200 : 100);
						fScore += gain;
						SpawnExplosion(enemy.x + 25, enemy.y + 24,
							enemy.kind == kEnemyBig ? 74 : 50);
						if (rand() % 4 == 0)
							SpawnItem(enemy.x + 25, enemy.y + 24);
					}
					break;
				}
			}
		} else if (Hits(bullet.x, bullet.y, 7, fPlayerX + 38,
			bounds.bottom - 96, 28)) {
			bullet.active = false;
			if (fShieldFrames == 0) {
				SpawnExplosion(fPlayerX + 38, bounds.bottom - 96, 66);
				if (--fLives <= 0)
					fGameOver = true;
				else
					fShieldFrames = 90;
			}
		}
	}

	for (int32 i = 0; i < kItemCount; i++) {
		Item& item = fItems[i];
		if (!item.active) continue;

		item.y += 1.6f;
		if (item.y > bounds.bottom) {
			item.active = false;
			continue;
		}
		if (Hits(item.x, item.y, 16, fPlayerX + 38,
			bounds.bottom - 96, 30)) {
			if (item.type == kItemPower && fPower < 3) fPower++;
			if (item.type == kItemShield) fShieldFrames = 240;
			fScore += 50;
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
			fBullets[i].y = Bounds().bottom - 132;
			fBullets[i].dy = -13;
			fBullets[i].enemy = false;
			fBullets[i].active = true;
			break;
		}
	}
	fFireFrames = fPower >= 3 ? 12 : 16;
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
