#include "GameView.h"

#include <Bitmap.h>
#include <Message.h>
#include <TranslationUtils.h>

#include <cstdio>
#include <math.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

enum { MSG_FRAME = 'fram' };

GameView::GameView()
	: BView(BRect(0, 0, 760, 600), "game", B_FOLLOW_ALL,
		B_WILL_DRAW | B_NAVIGABLE),
	  fShipSprite(NULL)
{
	SetViewColor(3, 7, 24);
	fShipSprite = BTranslationUtils::GetBitmapFile(
		"assets_cc0/space_shooter_pack/spritesheets/ship.png");
	srand((unsigned)time(NULL));
	ResetGame();
}

GameView::~GameView()
{
	delete fShipSprite;
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
	fGameOver = false;
	for (int32 i = 0; i < kBulletCount; i++) fBullets[i].active = false;
	for (int32 i = 0; i < kItemCount; i++) fItems[i].active = false;
	StartWave();
}

void GameView::StartWave()
{
	fWave++;
	for (int32 i = 0; i < kEnemyCount; i++) {
		Enemy& enemy = fEnemies[i];
		int32 row = i / 7;
		int32 col = i % 7;
		enemy.baseX = 74 + col * 94;
		enemy.x = enemy.baseX;
		enemy.y = 108 + row * 72;
		enemy.phase = col * .65f + row;
		enemy.hp = 1 + fWave / 3;
		enemy.alive = true;
	}
}

void GameView::Draw(BRect)
{
	BRect bounds = Bounds();
	SetHighColor(3, 7, 24);
	FillRect(bounds);
	DrawBackground(bounds);
	for (int32 i = 0; i < kEnemyCount; i++) if (fEnemies[i].alive) DrawEnemy(fEnemies[i]);
	DrawItems();
	DrawBullets();
	DrawPlayer(bounds);
	DrawHud(bounds);
	if (fGameOver) DrawGameOver(bounds);
}

void GameView::DrawBackground(BRect b)
{
	SetHighColor(22, 17, 68);
	FillEllipse(BRect(-110, 145, 340, 570));
	SetHighColor(54, 24, 120);
	StrokeEllipse(BRect(-110, 145, 340, 570));
	SetHighColor(10, 47, 92);
	FillEllipse(BRect(505, 130, 920, 550));
	SetHighColor(67, 180, 225);
	StrokeEllipse(BRect(505, 130, 920, 550));
	for (int32 i = 0; i < 145; i++) {
		float x = (i * 89 + 11) % (int32)b.Width();
		float y = 72 + ((i * 53 + 19) % (int32)(b.Height() - 110));
		int32 size = i % 13 == 0 ? 4 : 2;
		SetHighColor(i % 3 == 0 ? 130 : 200, i % 3 == 1 ? 220 : 244, 255);
		FillEllipse(BRect(x, y, x + size, y + size));
	}
	SetHighColor(18, 89, 142);
	for (int32 x = 0; x < b.right; x += 54)
		StrokeLine(BPoint(x, b.bottom - 28), BPoint(b.Width() / 2, 365));
	for (int32 y = 395; y < b.bottom - 25; y += 28)
		StrokeLine(BPoint(0, y), BPoint(b.right, y));
}

void GameView::DrawHud(BRect b)
{
	SetHighColor(7, 22, 57);
	FillRect(BRect(10, 10, b.right - 10, 64));
	SetHighColor(65, 230, 255);
	StrokeRect(BRect(10, 10, b.right - 10, 64));
	SetHighColor(80, 226, 255);
	SetFontSize(23);
	DrawString("NOVA ASSAULT", BPoint(24, 37));
	SetHighColor(164, 231, 255);
	SetFontSize(11);
	DrawString("A/D OR CLICK: STEER    AUTO-FIRE ONLINE    R: RESTART", BPoint(25, 55));
	char stats[100];
	snprintf(stats, sizeof(stats), "SCORE %ld   LIVES %ld   WAVE %ld   POWER x%ld",
		(long)fScore, (long)fLives, (long)fWave, (long)fPower);
	SetHighColor(255, 225, 93);
	SetFontSize(13);
	DrawString(stats, BPoint(b.right - 310, 37));
	SetHighColor(fShieldFrames > 0 ? 68 : 32, fShieldFrames > 0 ? 255 : 100, 230);
	DrawString(fShieldFrames > 0 ? "SHIELD ACTIVE" : "SHIELD OFFLINE", BPoint(b.right - 170, 55));
}

void GameView::DrawPlayer(BRect b)
{
	float x = fPlayerX;
	float y = b.bottom - 102;
	if (fShieldFrames > 0) {
		SetHighColor(45, 154, 255);
		StrokeEllipse(BRect(x - 26, y - 48, x + 92, y + 63));
	}
	if (fShipSprite != NULL) {
		DrawBitmap(fShipSprite, BRect(0, 0, 15, 23),
			BRect(x - 22, y - 47, x + 98, y + 133));
		return;
	}
	// Vector fallback if the CC0 sprite folder is missing.
	SetHighColor(102, 45, 255);
	FillTriangle(BPoint(x + 31, y + 47), BPoint(x + 38, y + 76), BPoint(x + 45, y + 47));
	FillTriangle(BPoint(x + 1, y + 43), BPoint(x + 6, y + 63), BPoint(x + 11, y + 43));
	FillTriangle(BPoint(x + 61, y + 43), BPoint(x + 66, y + 63), BPoint(x + 71, y + 43));
	SetHighColor(78, 16, 63);
	FillTriangle(BPoint(x + 38, y - 48), BPoint(x - 46, y + 50), BPoint(x + 122, y + 50));
	SetHighColor(224, 46, 84);
	FillTriangle(BPoint(x + 38, y - 43), BPoint(x - 34, y + 44), BPoint(x + 110, y + 44));
	SetHighColor(247, 244, 255);
	FillTriangle(BPoint(x + 38, y - 36), BPoint(x + 4, y + 39), BPoint(x + 72, y + 39));
	SetHighColor(145, 15, 57);
	FillTriangle(BPoint(x - 34, y + 44), BPoint(x - 7, y + 37), BPoint(x + 11, y + 59));
	FillTriangle(BPoint(x + 110, y + 44), BPoint(x + 83, y + 37), BPoint(x + 65, y + 59));
	SetHighColor(65, 206, 255);
	FillEllipse(BRect(x + 25, y - 20, x + 51, y + 13));
	SetHighColor(15, 55, 145);
	FillEllipse(BRect(x + 29, y - 16, x + 47, y + 7));
	SetHighColor(255, 212, 236);
	StrokeLine(BPoint(x - 46, y + 50), BPoint(x + 38, y - 48));
	StrokeLine(BPoint(x + 38, y - 48), BPoint(x + 122, y + 50));
}

void GameView::DrawEnemy(const Enemy& enemy)
{
	float x = enemy.x, y = enemy.y;
	SetHighColor(46, 21, 102);
	FillTriangle(BPoint(x + 25, y), BPoint(x - 8, y + 42), BPoint(x + 58, y + 42));
	SetHighColor(228, 64, 176);
	FillTriangle(BPoint(x + 25, y + 4), BPoint(x, y + 37), BPoint(x + 50, y + 37));
	SetHighColor(97, 235, 255);
	FillEllipse(BRect(x + 17, y + 13, x + 33, y + 28));
	SetHighColor(255, 210, 83);
	FillEllipse(BRect(x + 21, y + 30, x + 29, y + 38));
}

void GameView::DrawBullets()
{
	for (int32 i = 0; i < kBulletCount; i++) {
		if (!fBullets[i].active) continue;
		Bullet& bullet = fBullets[i];
		SetHighColor(bullet.enemy ? 255 : 67, bullet.enemy ? 81 : 183, bullet.enemy ? 125 : 255);
		FillEllipse(BRect(bullet.x - 4, bullet.y - 9, bullet.x + 4, bullet.y + 9));
		SetHighColor(255, 255, 255);
		FillEllipse(BRect(bullet.x - 1, bullet.y - 5, bullet.x + 1, bullet.y + 5));
	}
}

void GameView::DrawItems()
{
	for (int32 i = 0; i < kItemCount; i++) {
		if (!fItems[i].active) continue;
		Item& item = fItems[i];
		SetHighColor(item.type == 0 ? 255 : 54, item.type == 0 ? 200 : 243, item.type == 0 ? 59 : 205);
		FillEllipse(BRect(item.x - 15, item.y - 15, item.x + 15, item.y + 15));
		SetHighColor(255, 255, 225);
		SetFontSize(13);
		DrawString(item.type == 0 ? "P" : "S", BPoint(item.x - 4, item.y + 5));
	}
}

void GameView::DrawGameOver(BRect b)
{
	SetHighColor(44, 4, 25);
	FillRect(BRect(0, 75, b.right, b.bottom));
	SetHighColor(123, 16, 48);
	FillEllipse(BRect(-120, 112, 345, 588));
	SetHighColor(255, 78, 62);
	StrokeEllipse(BRect(-120, 112, 345, 588));
	SetHighColor(0, 0, 0, 155);
	FillRect(BRect(0, 78, b.right, b.bottom));
	SetHighColor(255, 90, 116);
	SetFontSize(34);
	DrawString("MISSION FAILED", BPoint(b.Width() / 2 - 144, 255));
	SetHighColor(230, 241, 255);
	SetFontSize(15);
	DrawString("Press R to restart the assault", BPoint(b.Width() / 2 - 112, 285));
}

void GameView::KeyDown(const char* bytes, int32)
{
	if (bytes[0] == 'a' || bytes[0] == 'A') fTargetX -= 50;
	else if (bytes[0] == 'd' || bytes[0] == 'D') fTargetX += 50;
	else if (bytes[0] == 'r' || bytes[0] == 'R') ResetGame();
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
	if (message->what == MSG_FRAME) UpdateGame();
	else BView::MessageReceived(message);
}

void GameView::UpdateGame()
{
	if (fGameOver) return;
	if (fPlayerX < fTargetX) fPlayerX += 7;
	if (fPlayerX > fTargetX) fPlayerX -= 7;
	if (fabs(fPlayerX - fTargetX) < 8) fPlayerX = fTargetX;
	if (fFireFrames > 0) fFireFrames--;
	if (fShieldFrames > 0) fShieldFrames--;
	if (fFireFrames == 0) FirePlayer();

	int32 alive = 0;
	for (int32 i = 0; i < kEnemyCount; i++) {
		Enemy& enemy = fEnemies[i];
		if (!enemy.alive) continue;
		alive++;
		enemy.phase += .04f;
		enemy.x = enemy.baseX + sin(enemy.phase) * 28;
		enemy.y += .03f + fWave * .006f;
		if (rand() % 240 == 0) FireEnemy(enemy);
	}
	if (alive == 0) StartWave();

	for (int32 i = 0; i < kBulletCount; i++) {
		Bullet& bullet = fBullets[i];
		if (!bullet.active) continue;
		bullet.y += bullet.dy;
		if (bullet.y < 70 || bullet.y > Bounds().bottom) { bullet.active = false; continue; }
		if (!bullet.enemy) {
			for (int32 e = 0; e < kEnemyCount; e++) {
				Enemy& enemy = fEnemies[e];
				if (enemy.alive && Hits(bullet.x, bullet.y, 6, enemy.x + 25, enemy.y + 22, 27)) {
					bullet.active = false;
					if (--enemy.hp <= 0) { enemy.alive = false; fScore += 100; if (rand() % 4 == 0) SpawnItem(enemy.x + 25, enemy.y + 22); }
					break;
				}
			}
		} else if (Hits(bullet.x, bullet.y, 7, fPlayerX + 38, Bounds().bottom - 85, 35)) {
			bullet.active = false;
			if (fShieldFrames == 0) { if (--fLives <= 0) fGameOver = true; else fShieldFrames = 45; }
		}
	}

	for (int32 i = 0; i < kItemCount; i++) {
		Item& item = fItems[i];
		if (!item.active) continue;
		item.y += 1.5f;
		if (item.y > Bounds().bottom) item.active = false;
		if (item.active && Hits(item.x, item.y, 15, fPlayerX + 38, Bounds().bottom - 85, 40)) {
			if (item.type == 0 && fPower < 3) fPower++;
			if (item.type == 1) fShieldFrames = 240;
			fScore += 50; item.active = false;
		}
	}
	Invalidate();
}

void GameView::FirePlayer()
{
	for (int32 n = 0; n < fPower; n++) {
		for (int32 i = 0; i < kBulletCount; i++) if (!fBullets[i].active) {
			fBullets[i].x = fPlayerX + 38 + (n - (fPower - 1) / 2.0f) * 18;
			fBullets[i].y = Bounds().bottom - 142;
			fBullets[i].dy = -12;
			fBullets[i].enemy = false;
			fBullets[i].active = true;
			break;
		}
	}
	fFireFrames = 17;
}

void GameView::FireEnemy(Enemy& enemy)
{
	for (int32 i = 0; i < kBulletCount; i++) if (!fBullets[i].active) {
		fBullets[i].x = enemy.x + 25;
		fBullets[i].y = enemy.y + 40;
		fBullets[i].dy = 5.2f;
		fBullets[i].enemy = true;
		fBullets[i].active = true;
		break;
	}
}

void GameView::SpawnItem(float x, float y)
{
	for (int32 i = 0; i < kItemCount; i++) if (!fItems[i].active) {
		fItems[i].x = x; fItems[i].y = y; fItems[i].type = rand() % 2; fItems[i].active = true; break;
	}
}

bool GameView::Hits(float ax, float ay, float ar, float bx, float by, float br) const
{
	float dx = ax - bx, dy = ay - by;
	return dx * dx + dy * dy < (ar + br) * (ar + br);
}
