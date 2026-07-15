#!/usr/bin/env python3
"""Plota diagramas polares a partir de níveis angulares medidos.

O script não estima o diagrama da APCA8090 a partir do ganho nominal. Ele
recebe níveis logarítmicos em dB por ângulo e normaliza cada curva ao próprio
máximo. O modo --example existe somente para validar a instalação.
"""

from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from math import cos, log10, pi
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = ROOT / "modelo_coleta_diagrama_radiacao.csv"
DEFAULT_OUTPUT = ROOT / "04-figuras" / "diagrama_radiacao_antena.png"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input",
        type=Path,
        default=DEFAULT_INPUT,
        help="CSV com plano, angulo_graus e nivel_db",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="arquivo PNG de saída",
    )
    parser.add_argument(
        "--floor-db",
        type=float,
        default=-30.0,
        help="piso do gráfico em dB relativos (padrão: -30)",
    )
    parser.add_argument(
        "--title",
        default="Resposta angular relativa",
        help="título do gráfico medido",
    )
    parser.add_argument(
        "--example",
        action="store_true",
        help="gera curvas genéricas apenas para testar o script",
    )
    return parser.parse_args()


def detect_dialect(path: Path) -> csv.Dialect:
    sample = path.read_text(encoding="utf-8-sig")[:4096]
    try:
        return csv.Sniffer().sniff(sample, delimiters=",;")
    except csv.Error:
        return csv.excel


def parse_number(text: str) -> float:
    return float(text.strip().replace(",", "."))


def load_measurements(path: Path) -> dict[str, list[tuple[float, float]]]:
    if not path.is_file():
        raise ValueError(f"arquivo de entrada não encontrado: {path}")

    groups: dict[str, list[tuple[float, float]]] = defaultdict(list)
    dialect = detect_dialect(path)
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream, dialect=dialect)
        required = {"plano", "angulo_graus"}
        if reader.fieldnames is None or not required.issubset(reader.fieldnames):
            raise ValueError(
                "o CSV deve conter as colunas plano, angulo_graus e nivel_db"
            )
        level_key = None
        for candidate in ("nivel_db", "nivel_relativo_db"):
            if candidate in reader.fieldnames:
                level_key = candidate
                break
        if level_key is None:
            raise ValueError(
                "o CSV deve conter a coluna nivel_db (ou nivel_relativo_db)"
            )
        for line_number, row in enumerate(reader, start=2):
            plane = (row.get("plano") or "").strip()
            angle_text = (row.get("angulo_graus") or "").strip()
            level_text = (row.get(level_key) or "").strip()
            if not plane or not angle_text or not level_text:
                continue
            try:
                angle = parse_number(angle_text) % 360.0
                level = parse_number(level_text)
            except ValueError as error:
                raise ValueError(f"linha {line_number}: valor numérico inválido") from error
            groups[plane].append((angle, level))

    groups = {
        plane: sorted(values)
        for plane, values in groups.items()
        if len(values) >= 3
    }
    if not groups:
        raise ValueError(
            "preencha pelo menos três ângulos de um plano antes de plotar"
        )
    return groups


def illustrative_measurements() -> dict[str, list[tuple[float, float]]]:
    angles = list(range(0, 360, 10))
    azimuth = []
    elevation = []
    for angle in angles:
        radians = angle * pi / 180.0
        # Quase circular, apenas para exercitar o desenho polar.
        omni_amplitude = max(0.05, 0.88 + 0.12 * cos(2.0 * radians))
        azimuth.append((angle, 20.0 * log10(omni_amplitude)))
        # Cardioide genérica; não representa a APCA8090.
        directional_amplitude = max(0.03, 0.5 * (1.0 + cos(radians)))
        elevation.append((angle, 20.0 * log10(directional_amplitude)))
    return {
        "Azimutal ilustrativo": azimuth,
        "Elevação ilustrativa": elevation,
    }


def normalized(groups: dict[str, list[tuple[float, float]]]) -> dict[str, list[tuple[float, float]]]:
    result = {}
    for plane, values in groups.items():
        maximum = max(level for _, level in values)
        result[plane] = [(angle, level - maximum) for angle, level in values]
    return result


def radial_ticks(floor_db: float) -> tuple[list[float], list[str]]:
    span = abs(floor_db)
    step = 5.0 if span <= 40 else 10.0
    levels = []
    current = floor_db
    while current <= 0.0001:
        levels.append(round(current, 6))
        current += step
    if levels[-1] != 0:
        levels.append(0.0)
    positions = [level - floor_db for level in levels]
    labels = [f"{level:g} dB" for level in levels]
    return positions, labels


def plot(
    groups: dict[str, list[tuple[float, float]]],
    output: Path,
    floor_db: float,
    title: str,
    example: bool,
) -> None:
    if floor_db >= 0:
        raise ValueError("--floor-db deve ser negativo")

    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    groups = normalized(groups)
    figure, axis = plt.subplots(
        figsize=(8.4, 8.0), subplot_kw={"projection": "polar"}, constrained_layout=True
    )

    for plane, values in groups.items():
        clipped = [(angle, max(floor_db, level)) for angle, level in values]
        angles = [angle * pi / 180.0 for angle, _ in clipped]
        radii = [level - floor_db for _, level in clipped]
        angles.append(angles[0] + 2.0 * pi)
        radii.append(radii[0])
        axis.plot(angles, radii, linewidth=2.3, marker="o", markersize=3.5, label=plane)

    axis.set_theta_zero_location("N")
    axis.set_theta_direction(-1)
    axis.set_thetagrids(range(0, 360, 30))
    positions, labels = radial_ticks(floor_db)
    axis.set_yticks(positions)
    axis.set_yticklabels(labels)
    axis.set_ylim(0, abs(floor_db))
    axis.set_rlabel_position(135)
    axis.grid(alpha=0.35)
    axis.legend(
        loc="upper center",
        bbox_to_anchor=(0.5, -0.08),
        ncol=min(2, len(groups)),
        frameon=False,
    )
    axis.set_title(
        "EXEMPLO ILUSTRATIVO — NÃO REPRESENTA A APCA8090" if example else title,
        pad=24,
        fontsize=14,
        fontweight="bold",
    )
    figure.text(
        0.5,
        -0.01,
        "Cada curva foi normalizada pelo próprio máximo (0 dB).",
        ha="center",
        fontsize=9,
    )

    output.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(output, dpi=180, bbox_inches="tight", facecolor="white")
    plt.close(figure)


def main() -> int:
    args = parse_args()
    try:
        groups = illustrative_measurements() if args.example else load_measurements(args.input)
        plot(groups, args.output, args.floor_db, args.title, args.example)
    except ValueError as error:
        print(f"erro: {error}", file=sys.stderr)
        return 2

    print(f"Diagrama gravado em: {args.output.resolve()}")
    if args.example:
        print("AVISO: curvas ilustrativas; não representam a antena APCA8090.")
    else:
        print("Dados normalizados por plano; máximo de cada série = 0 dB.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
