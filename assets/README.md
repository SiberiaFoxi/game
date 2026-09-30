# Модели мутантов — куда класть файлы

Мод грузит модели **из файлов, без правки gta3.img**:

```
assets/models/<ИмяМутанта>/model.dff
assets/models/<ИмяМутанта>/model.txd
```

`run_stalker.bat` копирует эту папку в `<игра>/STALKER_MOD/models/`.
Имена `<ИмяМутанта>` должны совпадать с секциями `mutants.cfg`
(`[mutant:blind_dog_1]` → `assets/models/blind_dog_1/`).

**Свободных ped-слотов всего 23** (неиспользуемые ID GTA SA). Порядок
назначения — по порядку секций в `mutants.cfg`. Остальные мутанты:
* замените `.dff`/`.txd` нужного педа в `gta3.img` программой IMG Tool
  и укажите `modelid=<ID>` в `mutants.cfg`;
* либо оставьте фолбэк (обычный пед с балансом мутанта).

## Паки моделей (скачать вручную)

| Пак | Ссылка | Что внутри |
| --- | --- | --- |
| Огромный сборник монстров из S.T.A.L.K.E.R. | [playground.ru](https://www.playground.ru/gta_san_andreas/file/grand_theft_auto_san_andreas_ogromnyj_sbornik_monstrov_iz_s_t_a_l_k_e_r-1040483) | 5 слепых псов, псевдопёс, пси-собака, кот-Баюн, кабан, плоть, 2 кровососа, 2 полтергейста, 2 излома, снорк, 2 контролёра, 10 зомби, тушканчик |
| Мутанты из S.T.A.L.K.E.R. ПЗ Ultimate v0.5.5 | [playground.ru](https://www.playground.ru/gta_san_andreas/file/grand_theft_auto_san_andreas_mutanty_iz_s_t_a_l_k_e_r_pz_ultimate_v0_5_5-1040477) | 26 мутантов |
| Pack Of Monsters (132 МБ) | [gta.com.ua](https://gta.com.ua/files/gta-san-andreas/12945) | псевдогигант (10 скинов), бюрер (15), химера (1) |
| Massive Monster Collection | [libertycity.net](https://libertycity.net/files/gta-san-andreas/93833-massive-monster-collection-from-s.t.a.l.k.e.r..html) | почти все монстры + своя анимация ходьбы четвероногих |
| Псевдогигант (10 вариаций, 20.84 МБ) | [libertycity.net](https://libertycity.net/files/gta-san-andreas/99056-pseudo-giant-from-s.t.a.l.k.e.r..html) | псевдогигант |
| Кровосос (5 разновидностей, 11.74 МБ) | [libertycity.net](https://libertycity.net/files/gta-san-andreas/95217-bloodsucker-from-s.t.a.l.k.e.r..html) | матёрый, обычный, зимний, болотная тварь, особый подвид |

**Авторы моделей:** Vadim_Panenko, Jagyar, TheLozza, mister_gad, =Borman=,
ПсевдоШрам, VIRTUS, Vadim Panenko, Borman. Указаны на страницах скачивания.
Уважайте их работу и лицензии паков!

## Как распаковать DFF/TXD из установщика пака

1. Паки часто идут как `.exe`-установщик или архив с готовыми файлами.
   Распакуйте архив (7-Zip открывает и установщики InnoSetup/NSIS).
2. Внутри обычно `gta3.img` (модифицированный) или отдельные `.dff`/`.txd`.
   Нужные вам `.dff`/`.txd` мутантов скопируйте в `assets/models/<имя>/`
   с переименованием в `model.dff` и `model.txd`.
3. Если пак содержит только `gta3.img` — смонтируйте/откройте его
   IMG Tool'ом и вытащите файлы вручную.
4. Имена файлов внутри паков (`bloodsucker.dff`, `boar.txd` и т.п.)
   переименуйте в `model.dff`/`model.txd`.

Модели паков сделаны под скелет педа — стандартные анимации ходьбы/боя
GTA SA применяются автоматически (ANIM_GROUP_MAN). Кастомные анимации
четвероногих (пак LibertyCity) в v1 не поддерживаются — используйте
стандартные.
