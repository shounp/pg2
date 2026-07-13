#!/usr/bin/env python3
"""Valida e consolida a planilha de simulação do protótipo RFID UHF.

Por padrão, o programa apenas lê a base sintética e recalcula suas métricas. A
escrita das tabelas e figuras exige confirmação explícita para evitar que as
saídas da simulação sejam confundidas com medições físicas do protótipo.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from pathlib import Path
from statistics import mean
import sys

from openpyxl import load_workbook


ROOT = Path(__file__).resolve().parents[1]
WORKBOOK = ROOT / "planilha_coleta_rfid_uhf_simulada.xlsx"
TABLE_OUTPUT = ROOT / "05-tabelas" / "resultados_rfid_gerados.tex"
FIGURE_DIR = ROOT / "04-figuras"
MATERIAL_LABELS = {
    "Papelao": "Papelão",
    "Plastico": "Plástico",
    "Metal com espacador": "Metal com espaçador",
}


def rows_as_dicts(workbook, sheet_name: str) -> list[dict[str, object]]:
    sheet = workbook[sheet_name]
    values = list(sheet.iter_rows(values_only=True))
    headers = [str(value) for value in values[0]]
    return [
        dict(zip(headers, row, strict=True))
        for row in values[1:]
        if any(value is not None for value in row)
    ]


def pt_number(value: float, decimals: int = 1) -> str:
    return f"{value:.{decimals}f}".replace(".", ",")


def aggregate_distance(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    groups: dict[float, list[dict[str, object]]] = defaultdict(list)
    for row in rows:
        groups[float(row["distancia_m"])].append(row)

    result = []
    for distance, group in sorted(groups.items()):
        attempts = sum(int(row["tentativas"]) for row in group)
        valid = sum(int(row["leituras_validas"]) for row in group)
        rates = [
            int(row["leituras_validas"]) / int(row["tentativas"]) * 100
            for row in group
        ]
        rssis = [float(row["rssi_medio"]) for row in group if row["rssi_medio"] is not None]
        result.append(
            {
                "distance": distance,
                "combinations": len(group),
                "attempts": attempts,
                "valid": valid,
                "rate": valid / attempts * 100,
                "minimum": min(rates),
                "maximum": max(rates),
                "rssi": mean(rssis) if rssis else None,
            }
        )
    return result


def aggregate_orientation(rows: list[dict[str, object]]) -> list[dict[str, float]]:
    groups: dict[float, list[dict[str, object]]] = defaultdict(list)
    for row in rows:
        groups[float(row["orientacao_graus"])].append(row)

    result = []
    for angle, group in sorted(groups.items()):
        attempts = sum(int(row["tentativas"]) for row in group)
        valid = sum(int(row["leituras_validas"]) for row in group)
        result.append({"angle": angle, "rate": valid / attempts * 100})
    return result


def aggregate_passage(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    groups: dict[float, list[dict[str, object]]] = defaultdict(list)
    for row in rows:
        groups[float(row["distancia_m"])].append(row)

    result = []
    for distance, group in sorted(groups.items()):
        attempts = sum(int(row["passagens"]) for row in group)
        valid = sum(int(row["passagens_detectadas"]) for row in group)
        rates = [
            int(row["passagens_detectadas"]) / int(row["passagens"]) * 100
            for row in group
        ]
        result.append(
            {
                "distance": distance,
                "rate": valid / attempts * 100,
                "minimum": min(rates),
                "maximum": max(rates),
            }
        )
    return result


def inventory_metrics(rows: list[dict[str, object]]) -> dict[str, object]:
    coverages = [
        int(row["qtd_tags_detectadas"]) / int(row["qtd_tags_presentes"]) * 100
        for row in rows
    ]
    complete_times = [
        float(row["tempo_ate_detectar_todas_s"])
        for row in rows
        if row["tempo_ate_detectar_todas_s"] is not None
    ]
    return {
        "rounds": len(rows),
        "tags": int(rows[0]["qtd_tags_presentes"]),
        "coverages": coverages,
        "mean_coverage": mean(coverages),
        "minimum_coverage": min(coverages),
        "maximum_coverage": max(coverages),
        "complete_rounds": sum(value == 100 for value in coverages),
        "mean_complete_time": mean(complete_times),
        "complete_time_n": len(complete_times),
        "external_reads": sum(int(row["leituras_externas"]) for row in rows),
        "mean_duplicates": mean(int(row["leituras_duplicadas"]) for row in rows),
    }


def integrity_warnings(workbook, sheets: dict[str, list[dict[str, object]]]) -> list[str]:
    warnings = []
    raw_rows = sheets["Dados_Brutos_Opcional"]
    summarized_attempts = (
        sum(int(row["tentativas"]) for row in sheets["Alcance_Altura_Dist"])
        + sum(int(row["tentativas"]) for row in sheets["Orientacao"])
        + sum(int(row["tentativas"]) for row in sheets["Material"])
        + sum(int(row["passagens"]) for row in sheets["Passagem"])
    )
    controlled_raw = sum(
        str(row["ensaio"]) != "inventario_sala_10s" for row in raw_rows
    )
    if controlled_raw != summarized_attempts:
        warnings.append(
            f"a aba bruta contém {controlled_raw} tentativas controladas para "
            f"{summarized_attempts} tentativas resumidas"
        )

    readme_text = " ".join(
        str(value)
        for row in workbook["README"].iter_rows(values_only=True)
        for value in row
        if value is not None
    ).upper()
    if "DADOS SINTÉTICOS" not in readme_text:
        warnings.append(
            "a planilha não contém aviso explícito de que os dados são sintéticos"
        )

    if "Simulados_Referencia" not in workbook.sheetnames:
        warnings.append(
            "a aba de documentação Simulados_Referencia não foi encontrada"
        )

    return warnings


def render_table(
    distance: list[dict[str, object]],
    material: list[dict[str, object]],
    inventory: dict[str, object],
) -> str:
    distance_rows = "\n".join(
        "        "
        f"{pt_number(float(row['distance']))} & {row['combinations']} & "
        f"{pt_number(float(row['rate']))}\\% & "
        f"{pt_number(float(row['minimum']))}\\%--{pt_number(float(row['maximum']))}\\% & "
        f"{pt_number(float(row['rssi']))} \\\\"
        for row in distance
    )
    material_rows = "\n".join(
        "        "
        f"{MATERIAL_LABELS.get(str(row['material/suporte']), row['material/suporte'])} & {row['leituras_validas']} & "
        f"{row['tentativas']} & "
        f"{pt_number(int(row['leituras_validas']) / int(row['tentativas']) * 100)}\\% & "
        f"{pt_number(float(row['rssi_medio']))} \\\\"
        for row in material
    )
    complete = int(inventory["complete_rounds"])
    rounds = int(inventory["rounds"])
    return f"""% Tabelas consolidadas a partir de planilha_coleta_rfid_uhf_simulada.xlsx.
% Todos os valores são sintéticos e não representam medições do protótipo.

\\begin{{table}}[htbp]
    \\centering
    \\footnotesize
    \\begin{{tabular}}{{ccccc}}
        \\toprule
        \\textbf{{Dist. horizontal (m)}} & \\textbf{{Combinações}} & \\textbf{{Taxa simulada}} & \\textbf{{Faixa}} & \\textbf{{RSSI sintético}} \\\\
        \\midrule
{distance_rows}
        \\bottomrule
    \\end{{tabular}}
    \\caption{{Resultado simulado por distância horizontal. Cada linha agrega 15 combinações tag--altura e 450 tentativas virtuais.}}
    \\label{{tab:resultado-alcance-distancia}}
\\end{{table}}

\\begin{{table}}[htbp]
    \\centering
    \\small
    \\begin{{tabularx}}{{\\textwidth}}{{Xcccc}}
        \\toprule
        \\textbf{{Material/suporte}} & \\textbf{{Detecções}} & \\textbf{{Tentativas}} & \\textbf{{Taxa simulada}} & \\textbf{{RSSI sintético}} \\\\
        \\midrule
{material_rows}
        \\bottomrule
    \\end{{tabularx}}
    \\caption{{Diferenças geradas pelo modelo para os materiais de fixação, com 30 tentativas virtuais por condição.}}
    \\label{{tab:resultado-material}}
\\end{{table}}

\\begin{{table}}[htbp]
    \\centering
    \\small
    \\begin{{tabularx}}{{\\textwidth}}{{Xc}}
        \\toprule
        \\textbf{{Métrica}} & \\textbf{{Valor}} \\\\
        \\midrule
        Rodadas analisadas & {rounds} \\\\
        Quantidade de tags por janela & {inventory['tags']} \\\\
        Cobertura média simulada & {pt_number(float(inventory['mean_coverage']))}\\% \\\\
        Faixa de cobertura & {pt_number(float(inventory['minimum_coverage']))}\\% a {pt_number(float(inventory['maximum_coverage']))}\\% \\\\
        Rodadas com cobertura completa & {complete}/{rounds} \\\\
        Tempo médio até detectar todas, nas janelas completas ($n={inventory['complete_time_n']}$) & {pt_number(float(inventory['mean_complete_time']))}~s \\\\
        Janelas sem cobertura completa no período & {rounds - complete}/{rounds} \\\\
        Leituras externas totais & {inventory['external_reads']} \\\\
        Duplicatas médias por janela & {pt_number(float(inventory['mean_duplicates']))} \\\\
        \\bottomrule
    \\end{{tabularx}}
    \\caption{{Resumo do cenário simulado de inventário em sala.}}
    \\label{{tab:resultado-inventario}}
\\end{{table}}
"""


def save_figure(figure, filename: str) -> None:
    path = FIGURE_DIR / filename
    figure.savefig(path, dpi=170, bbox_inches="tight", facecolor="white")


def write_figures(
    distance: list[dict[str, object]],
    height_rows: list[dict[str, object]],
    orientation: list[dict[str, float]],
    material: list[dict[str, object]],
    inventory: dict[str, object],
    passage: list[dict[str, object]],
) -> None:
    import matplotlib.pyplot as plt

    plt.rcParams.update(
        {
            "font.size": 12,
            "axes.labelsize": 14,
            "xtick.labelsize": 12,
            "ytick.labelsize": 12,
            "axes.grid": True,
            "grid.alpha": 0.25,
        }
    )

    x = [float(row["distance"]) for row in distance]
    y = [float(row["rate"]) for row in distance]
    lower = [float(row["minimum"]) for row in distance]
    upper = [float(row["maximum"]) for row in distance]
    fig, ax = plt.subplots(figsize=(10, 5.6), constrained_layout=True)
    ax.plot(x, y, marker="o", linewidth=2.6, color="#24658f")
    ax.fill_between(x, lower, upper, color="#24658f", alpha=0.17)
    ax.set(xlabel="Distância horizontal antena–tag (m)", ylabel="Taxa simulada de leitura (%)", ylim=(0, 105))
    save_figure(fig, "resultado_alcance_distancia.png")
    plt.close(fig)

    height_rows = sorted(height_rows, key=lambda row: int(row["altura_tag_cm"]))
    hx = [int(row["altura_tag_cm"]) for row in height_rows]
    hy = [int(row["leituras_validas"]) / int(row["tentativas"]) * 100 for row in height_rows]
    fig, ax = plt.subplots(figsize=(10, 5.6), constrained_layout=True)
    ax.plot(hx, hy, marker="o", linewidth=2.4, color="#813d18")
    ax.axvline(150, linestyle="--", linewidth=1.6, color="#444444", label="altura da antena")
    ax.set(xlabel="Altura da tag (cm)", ylabel="Taxa simulada a 2 m horizontais (%)", ylim=(0, 105))
    ax.legend(frameon=False)
    save_figure(fig, "resultado_alcance_altura_2m.png")
    plt.close(fig)

    angles = [int(row["angle"]) for row in orientation]
    angle_rates = [float(row["rate"]) for row in orientation]
    fig, ax = plt.subplots(figsize=(8.5, 5.6), constrained_layout=True)
    ax.bar([str(value) for value in angles], angle_rates, color="#348663")
    ax.set(xlabel="Orientação da tag (graus)", ylabel="Taxa simulada de leitura (%)", ylim=(0, 105))
    save_figure(fig, "resultado_orientacao.png")
    plt.close(fig)

    materials = [
        MATERIAL_LABELS.get(str(row["material/suporte"]), str(row["material/suporte"]))
        for row in material
    ]
    material_rates = [
        int(row["leituras_validas"]) / int(row["tentativas"]) * 100
        for row in material
    ]
    fig, ax = plt.subplots(figsize=(10, 5.8), constrained_layout=True)
    ax.bar(materials, material_rates, color="#7965a8")
    ax.set(ylabel="Taxa simulada de leitura (%)", ylim=(0, 105))
    ax.tick_params(axis="x", rotation=18)
    save_figure(fig, "resultado_material.png")
    plt.close(fig)

    rounds = list(range(1, len(inventory["coverages"]) + 1))
    fig, ax = plt.subplots(figsize=(10, 5.6), constrained_layout=True)
    ax.bar(rounds, inventory["coverages"], color="#b86139")
    ax.set(xlabel="Janela simulada de inventário", ylabel="Cobertura simulada (%)", ylim=(0, 105), xticks=rounds)
    save_figure(fig, "resultado_inventario.png")
    plt.close(fig)

    px = [float(row["distance"]) for row in passage]
    py = [float(row["rate"]) for row in passage]
    plower = [float(row["minimum"]) for row in passage]
    pupper = [float(row["maximum"]) for row in passage]
    fig, ax = plt.subplots(figsize=(10, 5.6), constrained_layout=True)
    ax.plot(px, py, marker="o", linewidth=2.6, color="#486b2b")
    ax.fill_between(px, plower, pupper, color="#486b2b", alpha=0.17)
    ax.set(xlabel="Distância lateral de passagem (m)", ylabel="Taxa simulada de detecção (%)", ylim=(0, 105))
    save_figure(fig, "resultado_passagem.png")
    plt.close(fig)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--write",
        action="store_true",
        help="grava a tabela LaTeX e as seis figuras",
    )
    parser.add_argument(
        "--acknowledge-simulated",
        action="store_true",
        help="confirma ciência de que todos os resultados são simulados",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.write and not args.acknowledge_simulated:
        print(
            "erro: --write exige --acknowledge-simulated",
            file=sys.stderr,
        )
        return 2

    workbook = load_workbook(WORKBOOK, data_only=True)
    names = [
        "Alcance_Altura_Dist",
        "Orientacao",
        "Material",
        "Inventario_Sala",
        "Passagem",
    ]
    sheets = {name: rows_as_dicts(workbook, name) for name in names}
    raw_sheet = (
        "Dados_Brutos"
        if "Dados_Brutos" in workbook.sheetnames
        else "Dados_Brutos_Opcional"
    )
    sheets["Dados_Brutos_Opcional"] = rows_as_dicts(workbook, raw_sheet)
    distance = aggregate_distance(sheets["Alcance_Altura_Dist"])
    orientation = aggregate_orientation(sheets["Orientacao"])
    inventory = inventory_metrics(sheets["Inventario_Sala"])
    passage = aggregate_passage(sheets["Passagem"])

    print("Métricas simuladas recalculadas:")
    for row in distance:
        print(
            f"  distância {row['distance']:.1f} m: "
            f"{row['valid']}/{row['attempts']} = {row['rate']:.1f}%"
        )
    print(
        f"  inventário: cobertura média {inventory['mean_coverage']:.1f}%; "
        f"{inventory['complete_rounds']}/{inventory['rounds']} janelas completas"
    )

    print(
        "NATUREZA DA BASE: dados integralmente sintéticos; "
        "não representam medições físicas."
    )
    warnings = integrity_warnings(workbook, sheets)
    for warning in warnings:
        print(f"ALERTA: {warning}", file=sys.stderr)

    if not args.write:
        print("Verificação da simulação concluída sem alterar arquivos.")
        return 0

    TABLE_OUTPUT.write_text(
        render_table(distance, sheets["Material"], inventory),
        encoding="utf-8",
        newline="\n",
    )
    height_rows = [
        row
        for row in sheets["Alcance_Altura_Dist"]
        if float(row["distancia_m"]) == 2.0
    ]
    write_figures(
        distance,
        height_rows,
        orientation,
        sheets["Material"],
        inventory,
        passage,
    )
    print(f"Tabela gravada em {TABLE_OUTPUT.relative_to(ROOT)}")
    print(f"Figuras gravadas em {FIGURE_DIR.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
