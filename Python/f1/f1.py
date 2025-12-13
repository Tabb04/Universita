import pandas as pd

# === Caricamento CSV ===
drivers = pd.read_csv("drivers.csv")
results = pd.read_csv("results.csv")
races = pd.read_csv("races.csv")
qualifying = pd.read_csv("qualifying.csv")
lap_times = pd.read_csv("lap_times.csv")


# ----------------------- Utilità -----------------------
def safe_pct(num, den):
    return (num / den * 100) if den else 0.0

def parse_years(input_str, career_years):
    career_years = sorted(set(int(y) for y in career_years))
    if input_str.strip().lower() == "tutti":
        return career_years
    years_selected = set()
    for part in input_str.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            try:
                start, end = map(int, part.split("-"))
                for y in range(start, end + 1):
                    if y in career_years:
                        years_selected.add(y)
            except ValueError:
                pass
        else:
            try:
                y = int(part)
                if y in career_years:
                    years_selected.add(y)
            except ValueError:
                pass
    return sorted(years_selected)

def choose_driver():
    while True:
        name_input = input("Inserisci il nome del pilota: ").strip().lower()
        if not name_input:
            print("❌ Inserisci un nome.")
            continue
        mask = (drivers["forename"].str.lower() + " " + drivers["surname"].str.lower()).str.contains(name_input)
        matched = drivers[mask]
        if matched.empty:
            print("❌ Pilota non trovato. Riprova.")
            continue
        if len(matched) > 1:
            print("Trovati più piloti:")
            for _, row in matched.iterrows():
                print(f"{row['driverId']}: {row['forename']} {row['surname']}")
            try:
                sel_id = int(input("Inserisci l'ID del pilota desiderato: "))
                if sel_id in matched["driverId"].values:
                    chosen = matched[matched["driverId"] == sel_id].iloc[0]
                    return int(chosen["driverId"]), chosen
                else:
                    print("❌ ID non presente tra i risultati.")
            except ValueError:
                print("❌ ID non valido.")
        else:
            row = matched.iloc[0]
            return int(row["driverId"]), row

def get_career_years(driverId):
    driver_results = results[results["driverId"] == driverId]
    years = races.loc[races["raceId"].isin(driver_results["raceId"]), "year"].unique()
    return sorted(int(y) for y in years)

def compute_stats(driverId, years, exclude_dnf=False):
    years = sorted(set(int(y) for y in years))
    selected_race_ids = set(races.loc[races["year"].isin(years), "raceId"].tolist())
    df_sel = results[(results["driverId"] == driverId) & (results["raceId"].isin(selected_race_ids))].copy()
    df_sel["position"] = pd.to_numeric(df_sel["position"], errors="coerce")

    df_fin = df_sel.dropna(subset=["position"]) if exclude_dnf else df_sel
    total_races = len(df_fin)
    total_wins = (df_fin["position"] == 1).sum()
    total_podiums = (df_fin["position"] <= 3).sum()
    total_points = df_fin["points"].sum() if "points" in df_fin.columns else None
    total_dnf = df_sel["position"].isna().sum()

    # === NEW: Average Finishing Position (solo gare concluse) ===
    avg_finish = df_fin["position"].mean() if not df_fin.empty else None

    allowed_race_ids = set(df_fin["raceId"].unique())
    total_poles = qualifying[(qualifying["driverId"] == driverId) & 
                             (qualifying["raceId"].isin(allowed_race_ids)) & 
                             (qualifying["position"] == 1)].shape[0]
    total_fastest_laps = 0
    for race_id in allowed_race_ids:
        laps_in_race = lap_times[lap_times["raceId"] == race_id]
        if laps_in_race.empty:
            continue
        min_time_all = laps_in_race["milliseconds"].min()
        driver_times = laps_in_race[laps_in_race["driverId"] == driverId]
        if not driver_times.empty and driver_times["milliseconds"].min() == min_time_all:
            total_fastest_laps += 1
    stats_per_year = {}
    for year in years:
        race_ids_year = set(races.loc[races["year"] == year, "raceId"].tolist())
        df_year_sel = df_sel[df_sel["raceId"].isin(race_ids_year)].copy()
        df_year_fin = df_year_sel.dropna(subset=["position"]) if exclude_dnf else df_year_sel
        races_year = len(df_year_fin)
        wins_year = (df_year_fin["position"] == 1).sum()
        podiums_year = (df_year_fin["position"] <= 3).sum()
        points_year = df_year_fin["points"].sum() if "points" in df_year_fin.columns else None
        dnf_year = df_year_sel["position"].isna().sum()
        avg_finish_year = df_year_fin["position"].mean() if not df_year_fin.empty else None  # NEW

        allowed_race_ids_year = set(df_year_fin["raceId"].unique())
        poles_year = qualifying[(qualifying["driverId"] == driverId) & 
                                (qualifying["raceId"].isin(allowed_race_ids_year)) & 
                                (qualifying["position"] == 1)].shape[0]
        fastest_year = 0
        for rid in allowed_race_ids_year:
            laps_in_race = lap_times[lap_times["raceId"] == rid]
            if laps_in_race.empty:
                continue
            min_time = laps_in_race["milliseconds"].min()
            driver_laps = laps_in_race[laps_in_race["driverId"] == driverId]
            if not driver_laps.empty and driver_laps["milliseconds"].min() == min_time:
                fastest_year += 1
        stats_per_year[year] = {
            "gare": races_year,
            "vittorie": wins_year,
            "podi": podiums_year,
            "dnf": dnf_year,
            "pole": poles_year,
            "giri_veloci": fastest_year,
            "punti": points_year,
            "avg_finish": avg_finish_year  # NEW
        }
    return {
        "years": years,
        "exclude_dnf": exclude_dnf,
        "totale": {
            "gare": total_races,
            "vittorie": total_wins,
            "podi": total_podiums,
            "dnf": total_dnf,
            "pole": total_poles,
            "giri_veloci": total_fastest_laps,
            "punti": total_points,
            "avg_finish": avg_finish  # NEW
        },
        "per_anno": dict(sorted(stats_per_year.items()))
    }

def print_stats(title, stats, show_percent=True, show_dnf=True):
    years = stats["years"]
    t = stats["totale"]
    print("\n" + "="*70)
    print(f"{title}")
    print("="*70)
    rng = f"{years[0]} - {years[-1]}" if years else "—"
    print(f"Intervallo anni: {rng}")
    print("\n— Totali —")
    print(f"Gare: {t['gare']}")
    print(f"Vittorie: {t['vittorie']}" + (f" ({safe_pct(t['vittorie'], t['gare']):.1f}%)" if show_percent else ""))
    print(f"Podi: {t['podi']}" + (f" ({safe_pct(t['podi'], t['gare']):.1f}%)" if show_percent else ""))
    if show_dnf:
        print(f"DNF: {t['dnf']}" + (f" ({safe_pct(t['dnf'], t['dnf'] + t['gare']):.1f}%)" if show_percent and (t['dnf'] + t['gare']) else ""))
    print(f"Pole position: {t['pole']}" + (f" ({safe_pct(t['pole'], t['gare']):.1f}%)" if show_percent else ""))
    print(f"Giri veloci: {t['giri_veloci']}" + (f" ({safe_pct(t['giri_veloci'], t['gare']):.1f}%)" if show_percent else ""))
    if t.get("punti") is not None:
        print(f"Punti: {t['punti']:.1f}")
    if t.get("avg_finish") is not None:   # NEW
        print(f"Average Finishing Position: {t['avg_finish']:.2f}")

    print("\n— Per stagione —")
    for year, s in stats["per_anno"].items():
        line = f"{year}: {s['gare']} gare | " \
               f"Vittorie: {s['vittorie']}" + (f" ({safe_pct(s['vittorie'], s['gare']):.1f}%) | " if s['gare'] else " | ")
        line += f"Podi: {s['podi']}" + (f" ({safe_pct(s['podi'], s['gare']):.1f}%) | " if s['gare'] else " | ")
        if show_dnf:
            line += f"DNF: {s['dnf']}" + (f" ({safe_pct(s['dnf'], s['dnf'] + s['gare']):.1f}%) | " if (s['dnf'] + s['gare']) else " | ")
        line += f"Pole: {s['pole']}" + (f" ({safe_pct(s['pole'], s['gare']):.1f}%) | " if s['gare'] else " | ")
        line += f"Giri veloci: {s['giri_veloci']}" + (f" ({safe_pct(s['giri_veloci'], s['gare']):.1f}%)" if s['gare'] else "")
        if s.get("punti") is not None:
            line += f" | Punti: {s['punti']:.1f}"
        if s.get("avg_finish") is not None:   # NEW
            line += f" | AvgFin: {s['avg_finish']:.2f}"
        print(line)




# ----------------------- Programma principale -----------------------
mode = input("Vuoi modalità singolo o confronto? (singolo/confronto): ").strip().lower()

if mode == "singolo":
    driverId, driver_row = choose_driver()
    driver_name = f"{driver_row['forename']} {driver_row['surname']}"
    career_years = get_career_years(driverId)

    print("\n📅 Anni di carriera:")
    print(", ".join(str(y) for y in career_years))

    sel = input("Inserisci anni da includere (es: 2008,2010-2012 oppure 'tutti'): ")
    selected_years = parse_years(sel, career_years)
    if not selected_years:
        print("❌ Nessun anno valido selezionato.")
        raise SystemExit(1)

    stats = compute_stats(driverId, selected_years, exclude_dnf=False)
    print_stats(f"📊 {driver_name} — Statistiche (con DNF nel totale gare)", stats, show_percent=True, show_dnf=True)

    no_dnf_choice = input("\nVuoi mostrare anche le statistiche ESCLUDENDO i DNF? (s/n): ").strip().lower()
    if no_dnf_choice == "s":
        stats_no_dnf = compute_stats(driverId, selected_years, exclude_dnf=True)
        print_stats(f"📊 {driver_name} — Statistiche (SENZA DNF)", stats_no_dnf, show_percent=True, show_dnf=False)

elif mode == "confronto":
    print("\n--- Primo pilota ---")
    d1_id, d1_row = choose_driver()
    n1 = f"{d1_row['forename']} {d1_row['surname']}"
    y1 = get_career_years(d1_id)

    print("\n--- Secondo pilota ---")
    d2_id, d2_row = choose_driver()
    n2 = f"{d2_row['forename']} {d2_row['surname']}"
    y2 = get_career_years(d2_id)

    # Statistiche carriera completa (ognuno sui propri anni)
    s1_full = compute_stats(d1_id, y1, exclude_dnf=False)
    s2_full = compute_stats(d2_id, y2, exclude_dnf=False)
    s1_no_dnf = compute_stats(d1_id, y1, exclude_dnf=True)
    s2_no_dnf = compute_stats(d2_id, y2, exclude_dnf=True)

    # --- Confronto carriera "riquadro" ---
    print("\n" + "="*70)
    print(f"🏁 Confronto carriera: {n1} | {n2}")
    print("="*70)
    stats_labels = ["vittorie", "podi", "dnf", "pole", "giri_veloci"]
    for label in stats_labels:
        v1, v2 = s1_full["totale"][label], s2_full["totale"][label]
        pct1 = safe_pct(v1, s1_full["totale"]["gare"])
        pct2 = safe_pct(v2, s2_full["totale"]["gare"])
        v1_nd, v2_nd = s1_no_dnf["totale"][label], s2_no_dnf["totale"][label]
        pct1_nd = safe_pct(v1_nd, s1_no_dnf["totale"]["gare"])
        pct2_nd = safe_pct(v2_nd, s2_no_dnf["totale"]["gare"])
        print(f"{label.capitalize():<12} | {n1}: {v1} ({pct1:.1f}%) | {n2}: {v2} ({pct2:.1f}%) | "
              f"SENZA DNF -> {n1}: {v1_nd} ({pct1_nd:.1f}%) | {n2}: {v2_nd} ({pct2_nd:.1f}%)")

    # NEW: confronto AvgFin
    af1, af2 = s1_full["totale"]["avg_finish"], s2_full["totale"]["avg_finish"]
    af1_nd, af2_nd = s1_no_dnf["totale"]["avg_finish"], s2_no_dnf["totale"]["avg_finish"]
    print(f"{'AvgFin':<12} | {n1}: {af1:.2f if af1 is not None else '—'} | {n2}: {af2:.2f if af2 is not None else '—'} | "
          f"SENZA DNF -> {n1}: {af1_nd:.2f if af1_nd is not None else '—'} | {n2}: {af2_nd:.2f if af2_nd is not None else '—'}")

    # --- Stagioni in comune ---
    common_years = sorted(set(y1) & set(y2))
    if not common_years:
        print("\n❌ Nessuna stagione in comune, confronto diretto non disponibile.")
    else:
        print("\n🔀 Stagioni in comune:", ", ".join(str(y) for y in common_years))

        # Confronto dettagliato piloti (anni comuni) — con e senza DNF
        for idx, (driver_id, name, years) in enumerate([(d1_id, n1, common_years), (d2_id, n2, common_years)]):
            stats_with_dnf = compute_stats(driver_id, years, exclude_dnf=False)
            stats_no_dnf = compute_stats(driver_id, years, exclude_dnf=True)

            print("\n" + "="*70)
            print(f"🤝 Confronto su anni comuni — {name}")
            print("="*70)
            print(f"Intervallo anni: {years[0]} - {years[-1]}")

            # Totali con DNF
            tot = stats_with_dnf["totale"]
            print("\n— Totali (con DNF) —")
            print(f"Gare: {tot['gare']}")
            print(f"Vittorie: {tot['vittorie']} ({safe_pct(tot['vittorie'], tot['gare']):.1f}%)")
            print(f"Podi: {tot['podi']} ({safe_pct(tot['podi'], tot['gare']):.1f}%)")
            print(f"DNF: {tot['dnf']} ({safe_pct(tot['dnf'], tot['gare']):.1f}%)")
            print(f"Pole position: {tot['pole']} ({safe_pct(tot['pole'], tot['gare']):.1f}%)")
            print(f"Giri veloci: {tot['giri_veloci']} ({safe_pct(tot['giri_veloci'], tot['gare']):.1f}%)")
            if tot.get("punti") is not None:
                print(f"Punti: {tot['punti']:.1f}")
            if tot.get("avg_finish") is not None:
                print(f"Average Finishing Position: {tot['avg_finish']:.2f}")

            # Per stagione con DNF
            print("\n— Per stagione (con DNF) —")
            for year in years:
                s = stats_with_dnf["per_anno"][year]
                line = (f"{year}: {s['gare']} gare | Vittorie: {s['vittorie']} ({safe_pct(s['vittorie'], s['gare']):.1f}%) | "
                        f"Podi: {s['podi']} ({safe_pct(s['podi'], s['gare']):.1f}%) | DNF: {s['dnf']} ({safe_pct(s['dnf'], s['gare']):.1f}%) | "
                        f"Pole: {s['pole']} ({safe_pct(s['pole'], s['gare']):.1f}%) | GV: {s['giri_veloci']} ({safe_pct(s['giri_veloci'], s['gare']):.1f}%)")
                if s.get("punti") is not None:
                    line += f" | Punti: {s['punti']:.1f}"
                if s.get("avg_finish") is not None:
                    line += f" | AvgFin: {s['avg_finish']:.2f}"
                print(line)

            # Per stagione senza DNF
            print("\n— Per stagione (SENZA DNF) —")
            for year in years:
                s = stats_no_dnf["per_anno"][year]
                line = (f"{year}: {s['gare']} gare | Vittorie: {s['vittorie']} ({safe_pct(s['vittorie'], s['gare']):.1f}%) | "
                        f"Podi: {s['podi']} ({safe_pct(s['podi'], s['gare']):.1f}%) | "
                        f"Pole: {s['pole']} ({safe_pct(s['pole'], s['gare']):.1f}%) | GV: {s['giri_veloci']} ({safe_pct(s['giri_veloci'], s['gare']):.1f}%)")
                if s.get("punti") is not None:
                    line += f" | Punti: {s['punti']:.1f}"
                if s.get("avg_finish") is not None:
                    line += f" | AvgFin: {s['avg_finish']:.2f}"
                print(line)

        # 📅 Confronto stagione per stagione (anni comuni) CON DNF
        s1_common = compute_stats(d1_id, common_years, exclude_dnf=False)
        s2_common = compute_stats(d2_id, common_years, exclude_dnf=False)
        print("\n📅 Confronto stagione per stagione (anni comuni) CON DNF")
        for year in common_years:
            a = s1_common["per_anno"][year]
            b = s2_common["per_anno"][year]
            print(f"{year} | {n1.split()[-1]} -> Vitt:{a['vittorie']} ({safe_pct(a['vittorie'], a['gare']):.1f}%) "
                  f"Podi:{a['podi']} ({safe_pct(a['podi'], a['gare']):.1f}%) DNF:{a['dnf']} ({safe_pct(a['dnf'], a['gare']):.1f}%) "
                  f"Pole:{a['pole']} ({safe_pct(a['pole'], a['gare']):.1f}%) GV:{a['giri_veloci']} ({safe_pct(a['giri_veloci'], a['gare']):.1f}%) "
                  f"AvgFin:{a['avg_finish']:.2f if a['avg_finish'] is not None else '—'} | "
                  f"{n2.split()[-1]} -> Vitt:{b['vittorie']} ({safe_pct(b['vittorie'], b['gare']):.1f}%) "
                  f"Podi:{b['podi']} ({safe_pct(b['podi'], b['gare']):.1f}%) DNF:{b['dnf']} ({safe_pct(b['dnf'], b['gare']):.1f}%) "
                  f"Pole:{b['pole']} ({safe_pct(b['pole'], b['gare']):.1f}%) GV:{b['giri_veloci']} ({safe_pct(b['giri_veloci'], b['gare']):.1f}%) "
                  f"AvgFin:{b['avg_finish']:.2f if b['avg_finish'] is not None else '—'}")

        # 📅 Confronto stagione per stagione (anni comuni) SENZA DNF
        s1_common_no_dnf = compute_stats(d1_id, common_years, exclude_dnf=True)
        s2_common_no_dnf = compute_stats(d2_id, common_years, exclude_dnf=True)
        print("\n📅 Confronto stagione per stagione (anni comuni) SENZA DNF")
        for year in common_years:
            a = s1_common_no_dnf["per_anno"][year]
            b = s2_common_no_dnf["per_anno"][year]
            print(f"{year} | {n1.split()[-1]} -> Vitt:{a['vittorie']} ({safe_pct(a['vittorie'], a['gare']):.1f}%) "
                  f"Podi:{a['podi']} ({safe_pct(a['podi'], a['gare']):.1f}%) Pole:{a['pole']} ({safe_pct(a['pole'], a['gare']):.1f}%) "
                  f"GV:{a['giri_veloci']} ({safe_pct(a['giri_veloci'], a['gare']):.1f}%) AvgFin:{a['avg_finish']:.2f if a['avg_finish'] is not None else '—'} | "
                  f"{n2.split()[-1]} -> Vitt:{b['vittorie']} ({safe_pct(b['vittorie'], b['gare']):.1f}%) "
                  f"Podi:{b['podi']} ({safe_pct(b['podi'], b['gare']):.1f}%) Pole:{b['pole']} ({safe_pct(b['pole'], b['gare']):.1f}%) "
                  f"GV:{b['giri_veloci']} ({safe_pct(b['giri_veloci'], b['gare']):.1f}%) AvgFin:{b['avg_finish']:.2f if b['avg_finish'] is not None else '—'}")
