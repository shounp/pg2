#!/usr/bin/env python3
"""Gera uma planilha limpa para uma campanha RFID UHF mínima e real."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from openpyxl import Workbook, load_workbook
from openpyxl.comments import Comment
from openpyxl.formatting.rule import CellIsRule
from openpyxl.styles import Alignment, Font, PatternFill, Border, Side
from openpyxl.utils import get_column_letter
from openpyxl.worksheet.datavalidation import DataValidation


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "planilha_coleta_rfid_uhf_experimento.xlsx"

NAVY = "1F4E78"
BLUE = "5B9BD5"
LIGHT_BLUE = "DDEBF7"
YELLOW = "FFF2CC"
GREEN = "E2F0D9"
RED = "F4CCCC"
GRAY = "E7E6E6"
WHITE = "FFFFFF"
THIN = Side(style="thin", color="B7B7B7")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="substitui somente a planilha experimental gerada anteriormente",
    )
    return parser.parse_args()


def style_header(sheet, row: int = 1) -> None:
    for cell in sheet[row]:
        if cell.value is None:
            continue
        cell.font = Font(bold=True, color=WHITE)
        cell.fill = PatternFill("solid", fgColor=BLUE)
        cell.alignment = Alignment(horizontal="center", vertical="center", wrap_text=True)
        cell.border = Border(bottom=THIN)
    sheet.row_dimensions[row].height = 34


def style_title(sheet, title: str, last_column: int) -> None:
    sheet.merge_cells(start_row=1, start_column=1, end_row=1, end_column=last_column)
    cell = sheet.cell(1, 1, title)
    cell.font = Font(size=16, bold=True, color=WHITE)
    cell.fill = PatternFill("solid", fgColor=NAVY)
    cell.alignment = Alignment(horizontal="center", vertical="center")
    sheet.row_dimensions[1].height = 28


def set_widths(sheet, widths: list[float]) -> None:
    for index, width in enumerate(widths, start=1):
        sheet.column_dimensions[get_column_letter(index)].width = width


def mark_input(cell, comment: str | None = None) -> None:
    cell.fill = PatternFill("solid", fgColor=YELLOW)
    if comment:
        cell.comment = Comment(comment, "Protocolo PG2")


def mark_formula(cell) -> None:
    cell.fill = PatternFill("solid", fgColor=LIGHT_BLUE)


def add_binary_validation(sheet, cell_range: str) -> None:
    validation = DataValidation(type="list", formula1='"0,1"', allow_blank=True)
    validation.error = "Digite somente 0 ou 1."
    validation.errorTitle = "Valor inválido"
    validation.prompt = "1 = detectou; 0 = não detectou."
    validation.promptTitle = "Resultado da tentativa"
    sheet.add_data_validation(validation)
    validation.add(cell_range)


def create_readme(workbook: Workbook) -> None:
    sheet = workbook.active
    sheet.title = "LEIA_ME"
    style_title(sheet, "PROTOCOLO EXPERIMENTAL MÍNIMO — PREENCHIMENTO OBRIGATÓRIO", 2)
    rows = [
        ("Natureza", "Planilha vazia para coleta física real. Nenhum resultado foi pré-preenchido."),
        ("Tempo total", "Planejada para aproximadamente 50–70 minutos, incluindo montagem, fotos e conferência."),
        ("Prioridade", "Execute nesta ordem: Configuração/fotos, Alcance, Orientação, Material e Inventário."),
        ("Tentativa", "Uma janela de 3 s. Retire a tag do campo por cerca de 2 s antes de cada nova tentativa."),
        ("Sucesso", "Preencha 1 somente se o EPC esperado aparecer ao menos uma vez durante a janela; caso contrário, 0."),
        ("Não inventar", "Não complete células esquecidas posteriormente por estimativa. Deixe em branco e explique em observações."),
        ("RSSI", "Não é obrigatório. Só registre se o firmware fornecer RSSI bruto ou uma conversão documentada."),
        ("Cor amarela", "Campo que deve ser preenchido durante ou imediatamente após a coleta."),
        ("Cor azul", "Célula calculada automaticamente; não digitar por cima."),
        ("Tags", "Cadastre cinco tags. TAG1, TAG2 e TAG3 são usadas nos testes controlados; as cinco entram no inventário."),
        ("Alcance", "4 distâncias × 3 tags × 5 tentativas = 60 janelas de 3 s."),
        ("Orientação", "3 ângulos × 3 tags × 5 tentativas = 45 janelas de 3 s, sempre a 1,5 m."),
        ("Material", "3 suportes × 5 tentativas = 15 janelas de 3 s, usando sempre TAG1."),
        ("Inventário", "5 janelas de 10 s com cinco tags e o mesmo percurso curto."),
        ("Passagem", "Foi retirada do protocolo para reduzir tempo e preservar a qualidade dos quatro ensaios centrais."),
        ("Backup", "Ao terminar, salve uma cópia com data/hora e preserve fotografias e qualquer log serial."),
    ]
    sheet.append(["Campo", "Instrução"])
    style_header(sheet, 2)
    for label, instruction in rows:
        sheet.append([label, instruction])
    for row in range(3, sheet.max_row + 1):
        sheet.cell(row, 1).font = Font(bold=True)
        sheet.cell(row, 2).alignment = Alignment(wrap_text=True, vertical="top")
        sheet.row_dimensions[row].height = 32
    set_widths(sheet, [24, 105])
    sheet.freeze_panes = "A3"
    sheet.sheet_view.showGridLines = False


def create_day_plan(workbook: Workbook) -> None:
    sheet = workbook.create_sheet("PLANO_DO_DIA")
    style_title(sheet, "ROTEIRO RÁPIDO DE EXECUÇÃO", 5)
    headers = ["Etapa", "Tempo", "O que fazer", "Critério para avançar", "Concluído (X)"]
    sheet.append(headers)
    style_header(sheet, 2)
    rows = [
        (1, "10 min", "Montar, conferir GND/fonte, abrir serial, cadastrar 5 EPCs e tirar fotos.", "Leitura de TAG1 confirmada e configuração preenchida."),
        (2, "20 min", "Executar as 60 janelas da aba Alcance, agrupadas por distância.", "Todas as células amarelas de detectou_0_1 preenchidas."),
        (3, "12 min", "Executar as 45 janelas da aba Orientacao em 0°, 45° e 90°.", "Ângulos fotografados ou marcados e resultados preenchidos."),
        (4, "8 min", "Executar papelão, metal direto e metal com espaçador usando TAG1.", "Cinco tentativas preenchidas em cada suporte."),
        (5, "10 min", "Executar cinco janelas de inventário de 10 s com o mesmo percurso.", "EPCs detectados e quantidade registrados em cada janela."),
        (6, "5 min", "Revisar campos vazios, salvar cópia e copiar fotos/logs.", "Arquivo abre novamente e a aba Resumo apresenta taxas."),
    ]
    for item in rows:
        sheet.append(item)
        mark_input(sheet.cell(sheet.max_row, 5))
    for row in range(3, sheet.max_row + 1):
        for column in range(1, 6):
            sheet.cell(row, column).alignment = Alignment(wrap_text=True, vertical="top")
        sheet.row_dimensions[row].height = 48
    set_widths(sheet, [10, 12, 66, 55, 16])
    sheet.freeze_panes = "A3"
    sheet.sheet_view.showGridLines = False


def create_configuration(workbook: Workbook) -> None:
    sheet = workbook.create_sheet("Configuracao")
    sheet.append(["Parâmetro", "Valor real", "Como preencher"])
    style_header(sheet)
    rows = [
        ("Data da coleta", None, "Data em que os experimentos foram realmente executados."),
        ("Hora de início", None, "Hora aproximada do início da montagem."),
        ("Local/sala", None, "Nome exato da sala e prédio."),
        ("Operador", "Leonardo Teixeira Gomes Silva", "Pessoa que realizou a coleta."),
        ("Leitor", "YPD-R200", "Confirmar a identificação do módulo."),
        ("Placa", None, "Arduino Uno ou Nano; informar qual foi usado."),
        ("Firmware/commit", None, "Nome/versão do arquivo carregado no Arduino."),
        ("Antena", "Airplux APCA8090", "Confirmar modelo/variante disponível."),
        ("Ganho nominal (dBi)", 5, "Valor nominal do fabricante."),
        ("Polarização", "Linear", "Polarização informada para a antena."),
        ("Região/canal/FHSS", None, "Registrar o que estiver configurado; se desconhecido, escrever NÃO VERIFICADO."),
        ("Potência configurada (dBm)", 26, "Registrar o comando usado; indicar se foi lido de volta."),
        ("Altura da antena (cm)", 150, "Medir do piso ao centro da antena."),
        ("Fonte do leitor", "5 V / 1 A externa", "Registrar tensão/corrente nominais da fonte."),
        ("Baud rate UART", 115200, "Velocidade entre Arduino e YPD-R200."),
        ("Intervalo de estabilização", None, "Tempo aguardado após mover tag/antena, em segundos."),
        ("Pessoas no ambiente", None, "Quantidade aproximada e movimentação durante o teste."),
        ("Objetos metálicos próximos", None, "Descrever bancada, armários e distância aproximada."),
        ("Fotos/arquivos", None, "Nomes das fotos e do log serial, se houver."),
        ("Ocorrências", None, "Reinicializações, quedas de alimentação ou mudanças de configuração."),
    ]
    for parameter, value, instruction in rows:
        sheet.append([parameter, value, instruction])
        mark_input(sheet.cell(sheet.max_row, 2))
        sheet.cell(sheet.max_row, 3).alignment = Alignment(wrap_text=True, vertical="top")
    set_widths(sheet, [32, 38, 72])
    sheet.freeze_panes = "A2"
    sheet.auto_filter.ref = f"A1:C{sheet.max_row}"


def create_tags(workbook: Workbook) -> None:
    sheet = workbook.create_sheet("Tags")
    headers = ["tag_id", "epc_real", "objeto/suporte", "material", "foto/posição", "observações"]
    sheet.append(headers)
    style_header(sheet)
    for index in range(1, 6):
        sheet.append([f"TAG{index}", None, None, None, None, None])
        for column in range(2, 7):
            mark_input(sheet.cell(sheet.max_row, column))
    set_widths(sheet, [12, 42, 28, 22, 28, 55])
    sheet.freeze_panes = "A2"
    sheet.auto_filter.ref = "A1:F6"


def create_repeated_test_sheet(
    workbook: Workbook,
    name: str,
    headers: list[str],
    rows: list[list[object]],
    input_columns: list[int],
    widths: list[float],
    binary_column: int,
) -> None:
    sheet = workbook.create_sheet(name)
    sheet.append(headers)
    style_header(sheet)
    for values in rows:
        sheet.append(values)
        for column in input_columns:
            mark_input(sheet.cell(sheet.max_row, column))
    last_column = get_column_letter(len(headers))
    sheet.freeze_panes = "A2"
    sheet.auto_filter.ref = f"A1:{last_column}{sheet.max_row}"
    set_widths(sheet, widths)
    add_binary_validation(sheet, f"{get_column_letter(binary_column)}2:{get_column_letter(binary_column)}{sheet.max_row}")
    result_range = f"{get_column_letter(binary_column)}2:{get_column_letter(binary_column)}{sheet.max_row}"
    sheet.conditional_formatting.add(
        result_range,
        CellIsRule(operator="equal", formula=["0"], fill=PatternFill("solid", fgColor=RED)),
    )
    sheet.conditional_formatting.add(
        result_range,
        CellIsRule(operator="equal", formula=["1"], fill=PatternFill("solid", fgColor=GREEN)),
    )


def create_distance(workbook: Workbook) -> None:
    headers = [
        "ordem", "distancia_horizontal_m", "tag_id", "tentativa", "altura_antena_cm",
        "altura_tag_cm", "orientacao_graus", "material_suporte", "duracao_janela_s",
        "detectou_0_1", "epc_observado", "tempo_primeira_leitura_s", "observacoes",
    ]
    rows = []
    order = 1
    for distance in (0.5, 1.0, 1.5, 2.0):
        for tag in ("TAG1", "TAG2", "TAG3"):
            for attempt in range(1, 6):
                rows.append([order, distance, tag, attempt, 150, 150, 0, "Papelao", 3, None, None, None, None])
                order += 1
    create_repeated_test_sheet(
        workbook, "Alcance", headers, rows, [10, 11, 12, 13],
        [9, 20, 11, 10, 18, 16, 18, 21, 18, 16, 38, 24, 55], 10,
    )


def create_orientation(workbook: Workbook) -> None:
    headers = [
        "ordem", "orientacao_graus", "tag_id", "tentativa", "distancia_m",
        "altura_antena_cm", "altura_tag_cm", "material_suporte", "duracao_janela_s",
        "detectou_0_1", "epc_observado", "tempo_primeira_leitura_s", "observacoes",
    ]
    rows = []
    order = 1
    for angle in (0, 45, 90):
        for tag in ("TAG1", "TAG2", "TAG3"):
            for attempt in range(1, 6):
                rows.append([order, angle, tag, attempt, 1.5, 150, 150, "Papelao", 3, None, None, None, None])
                order += 1
    create_repeated_test_sheet(
        workbook, "Orientacao", headers, rows, [10, 11, 12, 13],
        [9, 18, 11, 10, 15, 18, 16, 21, 18, 16, 38, 24, 55], 10,
    )


def create_material(workbook: Workbook) -> None:
    headers = [
        "ordem", "material_suporte", "espacador_mm", "tag_id", "tentativa",
        "distancia_m", "altura_antena_cm", "altura_tag_cm", "orientacao_graus",
        "duracao_janela_s", "detectou_0_1", "epc_observado",
        "tempo_primeira_leitura_s", "observacoes",
    ]
    rows = []
    order = 1
    for material, spacer in (("Papelao", 0), ("Metal direto", 0), ("Metal com espacador", 10)):
        for attempt in range(1, 6):
            rows.append([order, material, spacer, "TAG1", attempt, 1.5, 150, 150, 0, 3, None, None, None, None])
            order += 1
    create_repeated_test_sheet(
        workbook, "Material", headers, rows, [11, 12, 13, 14],
        [9, 25, 16, 11, 10, 15, 18, 16, 18, 18, 16, 38, 24, 55], 11,
    )


def create_inventory(workbook: Workbook) -> None:
    sheet = workbook.create_sheet("Inventario")
    headers = [
        "rodada", "duracao_janela_s", "qtd_tags_presentes", "epcs_detectados_separados_por_virgula",
        "qtd_tags_detectadas", "cobertura_%", "completa_0_1", "leituras_externas",
        "percurso", "observacoes",
    ]
    sheet.append(headers)
    style_header(sheet)
    for round_number in range(1, 6):
        row = sheet.max_row + 1
        sheet.append([round_number, 10, 5, None, None, f'=IF(C{row}>0,E{row}/C{row}*100,"")', f'=IF(E{row}="","",--(E{row}=C{row}))', None, "Percurso curto padronizado", None])
        for column in (4, 5, 8, 9, 10):
            mark_input(sheet.cell(row, column))
        mark_formula(sheet.cell(row, 6))
        mark_formula(sheet.cell(row, 7))
        sheet.cell(row, 6).number_format = "0.0"
    add_binary_validation(sheet, "G2:G6")
    set_widths(sheet, [10, 20, 20, 58, 22, 16, 16, 18, 32, 55])
    sheet.freeze_panes = "A2"
    sheet.auto_filter.ref = "A1:J6"


def create_raw_log(workbook: Workbook) -> None:
    sheet = workbook.create_sheet("Log_Serial_Opcional")
    headers = ["timestamp", "ensaio", "condicao", "tentativa", "epc", "leitura_valida_0_1", "linha_serial_original", "observacoes"]
    sheet.append(headers)
    style_header(sheet)
    for row in range(2, 202):
        for column in range(1, 9):
            mark_input(sheet.cell(row, column))
    add_binary_validation(sheet, "F2:F201")
    set_widths(sheet, [24, 20, 28, 12, 42, 22, 80, 50])
    sheet.freeze_panes = "A2"
    sheet.auto_filter.ref = "A1:H201"


def create_summary(workbook: Workbook) -> None:
    sheet = workbook.create_sheet("Resumo", 2)
    style_title(sheet, "RESUMO AUTOMÁTICO — NÃO DIGITAR NAS CÉLULAS AZUIS", 7)
    headers = ["Ensaio", "Condição", "Planejado", "Preenchido", "Sucessos", "Taxa_%", "Situação"]
    sheet.append(headers)
    style_header(sheet, 2)
    row = 3
    for distance in (0.5, 1.0, 1.5, 2.0):
        sheet.cell(row, 1, "Alcance")
        sheet.cell(row, 2, distance)
        sheet.cell(row, 2).number_format = '0.0" m"'
        sheet.cell(row, 3, 15)
        sheet.cell(row, 4, f'=COUNTIFS(Alcance!$B:$B,B{row},Alcance!$J:$J,"<>")')
        sheet.cell(row, 5, f'=COUNTIFS(Alcance!$B:$B,B{row},Alcance!$J:$J,1)')
        sheet.cell(row, 6, f'=IF(D{row}>0,E{row}/D{row}*100,"")')
        sheet.cell(row, 7, f'=IF(D{row}=C{row},"COMPLETO","PENDENTE")')
        row += 1
    for angle in (0, 45, 90):
        sheet.cell(row, 1, "Orientacao")
        sheet.cell(row, 2, angle)
        sheet.cell(row, 2).number_format = '0"°"'
        sheet.cell(row, 3, 15)
        sheet.cell(row, 4, f'=COUNTIFS(Orientacao!$B:$B,B{row},Orientacao!$J:$J,"<>")')
        sheet.cell(row, 5, f'=COUNTIFS(Orientacao!$B:$B,B{row},Orientacao!$J:$J,1)')
        sheet.cell(row, 6, f'=IF(D{row}>0,E{row}/D{row}*100,"")')
        sheet.cell(row, 7, f'=IF(D{row}=C{row},"COMPLETO","PENDENTE")')
        row += 1
    for material in ("Papelao", "Metal direto", "Metal com espacador"):
        sheet.cell(row, 1, "Material")
        sheet.cell(row, 2, material)
        sheet.cell(row, 3, 5)
        sheet.cell(row, 4, f'=COUNTIFS(Material!$B:$B,B{row},Material!$K:$K,"<>")')
        sheet.cell(row, 5, f'=COUNTIFS(Material!$B:$B,B{row},Material!$K:$K,1)')
        sheet.cell(row, 6, f'=IF(D{row}>0,E{row}/D{row}*100,"")')
        sheet.cell(row, 7, f'=IF(D{row}=C{row},"COMPLETO","PENDENTE")')
        row += 1
    sheet.cell(row, 1, "Inventario")
    sheet.cell(row, 2, "5 janelas")
    sheet.cell(row, 3, 5)
    sheet.cell(row, 4, '=COUNT(Inventario!$E$2:$E$6)')
    sheet.cell(row, 5, '=SUM(Inventario!$G$2:$G$6)')
    sheet.cell(row, 6, '=IFERROR(AVERAGE(Inventario!$F$2:$F$6),"")')
    sheet.cell(row, 7, f'=IF(D{row}=C{row},"COMPLETO","PENDENTE")')

    for data_row in range(3, row + 1):
        for column in range(4, 8):
            mark_formula(sheet.cell(data_row, column))
        sheet.cell(data_row, 6).number_format = "0.0"
    set_widths(sheet, [18, 28, 14, 14, 14, 14, 18])
    sheet.freeze_panes = "A3"
    sheet.auto_filter.ref = f"A2:G{row}"


def main() -> int:
    args = parse_args()
    if OUTPUT.exists() and not args.overwrite:
        print(f"erro: {OUTPUT.name} já existe; use --overwrite para regenerar", file=sys.stderr)
        return 2

    workbook = Workbook()
    create_readme(workbook)
    create_day_plan(workbook)
    create_summary(workbook)
    create_configuration(workbook)
    create_tags(workbook)
    create_distance(workbook)
    create_orientation(workbook)
    create_material(workbook)
    create_inventory(workbook)
    create_raw_log(workbook)

    workbook.properties.title = "Coleta experimental mínima RFID UHF"
    workbook.properties.subject = "Planilha vazia para medições físicas do PG2"
    workbook.properties.description = (
        "Protocolo reduzido: alcance, orientação, material e inventário. "
        "Nenhum resultado experimental pré-preenchido."
    )
    workbook.properties.creator = "Projeto de Graduação 2"
    workbook.save(OUTPUT)

    # Confirma que todas as células de resultado permanecem vazias.
    check = load_workbook(OUTPUT, data_only=False)
    required_ranges = {
        "Alcance": ("J", 2, 61),
        "Orientacao": ("J", 2, 46),
        "Material": ("K", 2, 16),
        "Inventario": ("E", 2, 6),
    }
    for sheet_name, (column, first, last) in required_ranges.items():
        if any(check[sheet_name][f"{column}{row}"].value is not None for row in range(first, last + 1)):
            raise RuntimeError(f"resultado pré-preenchido detectado em {sheet_name}")

    print(f"Planilha criada: {OUTPUT}")
    print("Resultados pré-preenchidos: 0")
    print("Carga principal: 60 alcance + 45 orientação + 15 material + 5 inventários")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
