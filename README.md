# Paleta vlastností – doplněk pro Archicad 29 (Windows)

Paleta **Okno › Paletky › Vlastnosti prvku** zobrazuje vlastnosti vybraných prvků
ve stejném členění jako Správce vlastností a umožňuje je rovnou upravovat.

## Co umí

- Při každé změně výběru se paleta sama obnoví.
- Vlastnosti jsou seskupené podle skupin: nejdřív vlastní skupiny, pak vestavěné.
  Kliknutím na řádek skupiny ji sbalíte nebo rozbalíte.
- **Hledat:** filtruje podle názvu vlastnosti, hodnoty i názvu skupiny.
- **Vestavěné:** zapíná a vypíná zobrazení vestavěných vlastností Archicadu.
- U víc vybraných prvků se zobrazí jen vlastnosti, které mají všechny prvky.
  Rozdílné hodnoty jsou označené jako „‹různé›“.
- **Úpravy:**
  - text a čísla se píšou do pole pod seznamem a potvrzují tlačítkem **Použít** nebo klávesou Enter,
  - Ano/Ne a výběr z možností se vybírají v rozbalovacím seznamu,
  - **Výchozí** vrátí vlastní vlastnost na její výchozí hodnotu (případně výraz),
  - změna se zapíše do všech vybraných prvků a jde vrátit přes Zpět (Ctrl+Z).
- Šedé řádky jsou jen pro čtení: vlastnosti počítané výrazem, vestavěné vlastnosti
  bez možnosti úpravy a prvky z hotlinku.
- Čísla se zadávají v jednotkách z *Předvolby projektu › Jednotky výpočtu*.
  Desetinná čárka i tečka fungují.
- Paleta funguje v půdorysu, řezech, pohledech, 3D, detailech, výkresech i v layoutu.

## Omezení první verze

- Vlastnosti typu **seznam** a **vícenásobný výběr** paleta jen zobrazí.
  Upravují se dál v Nastavení prvku.
- U výběru větším než 200 prvků se vlastnosti načtou z prvních 200 prvků.
  Zapisuje se ale do všech vybraných.
- V Teamworku jde upravit jen rezervované prvky. U ostatních paleta vypíše, kolik
  prvků nešlo změnit.

## Sestavení

Archicad načítá doplňky jako soubor `.apx`. Ten vznikne sestavením (kompilací) tohoto
projektu. Jsou dvě cesty:

### A) Bez instalace čehokoliv – GitHub Actions (doporučeno)

1. Založte si zdarma účet na <https://github.com>.
2. Vytvořte nový **soukromý** repozitář (tlačítko *New*), třeba `PaletaVlastnosti`.
3. Na stránce repozitáře zvolte *uploading an existing file* a přetáhněte do prohlížeče
   **celý obsah** této složky, včetně složek `.github`, `Src`, `Tools`, `RINT`, `RFIX`
   a `RFIX.win`. Potvrďte *Commit changes*.
   - Složka `.github` je skrytá. Ve Windows ji zobrazíte v Průzkumníku přes
     *Zobrazit › Skryté položky*.
4. Otevřete záložku **Actions**. Sestavení se spustí samo a trvá zhruba 5–10 minut.
5. Po dokončení (zelená fajfka) klikněte na běh a dole v části *Artifacts* stáhněte
   `PaletaVlastnosti-AC29-Windows`. V ZIPu je soubor `PaletaVlastnosti.apx`.

### B) Na vlastním počítači

Potřebujete:

- **Visual Studio 2022** (stačí bezplatná edice Community nebo *Build Tools*) se sadou
  *Vývoj desktopových aplikací pomocí C++* (Desktop development with C++),
- **CMake** (je součástí výše uvedené sady),
- **Python 3.10+** z <https://www.python.org> (při instalaci zaškrtněte *Add python.exe to PATH*).

Pak dvakrát klikněte na `Sestavit.bat`. Skript si sám stáhne Archicad API Development
Kit 29, sestaví doplněk a výsledný `PaletaVlastnosti.apx` uloží vedle sebe.

## Instalace do Archicadu

1. Zkopírujte `PaletaVlastnosti.apx` do libovolné složky, např. `Dokumenty\Archicad doplňky`.
2. V Archicadu otevřete **Možnosti › Správce doplňků**.
3. Klikněte na **Přidat…**, vyberte soubor `.apx` a potvrďte.
4. Paletu otevřete přes **Okno › Paletky › Vlastnosti prvku**. Archicad si pamatuje,
   jestli byla otevřená, a při dalším spuštění ji obnoví.

Pokud Správce doplňků hlásí chybu načtení, pošlete text chyby. Opravím to.

## Struktura projektu

| Soubor | Obsah |
|---|---|
| `Src/AddOnMain.cpp` | registrace doplňku, menu a notifikací |
| `Src/PropertyPalette.*` | paleta (rozhraní, seznam, editor hodnot) |
| `Src/PropertyData.*` | čtení a zápis vlastností přes Archicad API |
| `RINT/AddOn.grc` | rozvržení palety a české texty |
| `Tools/` | oficiální sestavovací nástroje Graphisoftu (archicad-addon-cmake) |
