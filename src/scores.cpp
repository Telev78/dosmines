#include <stdio.h>
#include <string.h>
#include <dir.h>
#include "scores.h"

#define SCORE_FILE "SCORES.DAT"

ScoreManager::ScoreManager() : saveFailed(0)
{
    strcpy(path, SCORE_FILE);
    reset();
}

/* exePath = argv[0]. Le fichier de scores est place a cote de l'executable. */
void ScoreManager::init(const char *exePath)
{
    char drive[MAXDRIVE], dir[MAXDIR], name[MAXFILE], ext[MAXEXT];

    strcpy(path, SCORE_FILE);
    if (exePath && exePath[0])
    {
        fnsplit(exePath, drive, dir, name, ext);
        fnmerge(path, drive, dir, "SCORES", ".DAT");
    }
    reset();
    load();
}

void ScoreManager::reset()
{
    strcpy(scores[0].name, STR_MENU_SCORES_DEFAULT);
    scores[0].time = 999;

    strcpy(scores[1].name, STR_MENU_SCORES_DEFAULT);
    scores[1].time = 999;

    strcpy(scores[2].name, STR_MENU_SCORES_DEFAULT);
    scores[2].time = 999;
}

void ScoreManager::load()
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return;

    if (fread(scores, sizeof(HighScore), 3, f) != 3)
    {
        fclose(f);
        reset(); /* fichier tronque ou corrompu */
        return;
    }
    fclose(f);

    /* Verifications de securite sur les valeurs chargees */
    for (int i = 0; i < 3; i++)
    {
        scores[i].name[15] = '\0';
        if (scores[i].time < 1)
            scores[i].time = 1;
        if (scores[i].time > 999)
            scores[i].time = 999;
    }
}

void ScoreManager::save()
{
    FILE *f = fopen(path, "wb");
    if (!f)
    {
        saveFailed = 1;
        return;
    }
    int ok = (fwrite(scores, sizeof(HighScore), 3, f) == 3);
    if (fclose(f) != 0)
        ok = 0;
    saveFailed = !ok;
}

int ScoreManager::isNewRecord(Difficulty d, int seconds) const
{
    int idx = (int)d;
    if (idx < 0 || idx > 2)
        return 0;
    return (seconds < scores[idx].time);
}

void ScoreManager::update(Difficulty d, const char *name, int seconds)
{
    int idx = (int)d;
    if (idx < 0 || idx > 2)
        return;

    strncpy(scores[idx].name, name, 15);
    scores[idx].name[15] = '\0';
    if (scores[idx].name[0] == '\0')
    {
        strcpy(scores[idx].name, STR_MENU_SCORES_DEFAULT);
    }
    scores[idx].time = seconds;
    save();
}

const HighScore &ScoreManager::get(Difficulty d) const
{
    int idx = (int)d;
    if (idx < 0 || idx > 2)
        idx = 0;
    return scores[idx];
}
