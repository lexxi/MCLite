"""Export an OSM PBF to three GeoPackage tables inside QGIS 3.40.

Run via exec(compile(open(...).read(), ..., 'exec')) in the QGIS console.
Edit SOURCE and OUTPUT below for another region. No clipping or styling occurs.
"""
from pathlib import Path
from uuid import uuid4
import gc

from qgis.core import QgsProject, QgsVectorLayer, QgsVectorFileWriter

SOURCE = Path(r"C:\pyprojects\MCLite-Maps\source\austria-latest.osm.pbf")
OUTPUT = Path(r"C:\pyprojects\MCLite-Maps\source\oesterreich-auto.gpkg")
TABLES = ("points", "lines", "multipolygons")


def export_osm(source, output):
    source, output = Path(source), Path(output)
    if not source.is_file():
        raise FileNotFoundError(f"PBF not found: {source}")
    if output.exists():
        raise FileExistsError(f"Output already exists; choose another OUTPUT: {output}")
    if output.suffix.lower() != ".gpkg":
        raise ValueError("OUTPUT must end in .gpkg")
    output.parent.mkdir(parents=True, exist_ok=True)
    partial = output.with_name(f"{output.stem}.partial-{uuid4().hex}.gpkg")
    project = QgsProject.instance()
    print(f"Export started: {source}\nTemporary output: {partial}", flush=True)
    try:
        for index, table in enumerate(TABLES):
            print(f"[{index + 1}/3] Exporting {table} ...", flush=True)
            layer = QgsVectorLayer(f"{source}|layername={table}", table, "ogr")
            if not layer.isValid():
                raise RuntimeError(f"Cannot open PBF layer: {table}")
            options = QgsVectorFileWriter.SaveVectorOptions()
            options.driverName = "GPKG"
            options.fileEncoding = "UTF-8"
            options.layerName = table
            options.onlySelectedFeatures = False
            options.actionOnExistingFile = (
                QgsVectorFileWriter.CreateOrOverwriteFile if index == 0
                else QgsVectorFileWriter.CreateOrOverwriteLayer
            )
            result = QgsVectorFileWriter.writeAsVectorFormatV3(
                layer, str(partial), project.transformContext(), options
            )
            del layer
            if result[0] != QgsVectorFileWriter.NoError:
                raise RuntimeError(f"Export failed for {table}: {result}")
            check = QgsVectorLayer(f"{partial}|layername={table}", table, "ogr")
            if not check.isValid():
                raise RuntimeError(f"Exported table cannot be opened: {table}")
            del check
            gc.collect()
            print(f"[{index + 1}/3] {table} finished.", flush=True)
        if output.exists():
            raise FileExistsError(f"Output appeared during export: {output}")
        partial.rename(output)
    except Exception:
        print(f"Export stopped. Any partial result remains at: {partial}", flush=True)
        raise

    # Load only the completed file. Rendering stays disabled for large datasets.
    for table in reversed(TABLES):
        layer = QgsVectorLayer(f"{output}|layername={table}", table, "ogr")
        if not layer.isValid():
            raise RuntimeError(f"Cannot load completed table {table} from {output}")
        project.addMapLayer(layer, False)
        node = project.layerTreeRoot().insertLayer(0, layer)
        node.setItemVisibilityChecked(False)
    print(f"Finished: {output}\nThree layers loaded, visibility off. Save the project manually.", flush=True)


export_osm(SOURCE, OUTPUT)
