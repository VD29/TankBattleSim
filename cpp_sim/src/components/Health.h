#pragma once

struct Health {
    int hp    = 100;
    int maxHp = 100;
    int armor = 0;   // flat damage reduction applied before hp loss

    bool isDead() const { return hp <= 0; }
};
