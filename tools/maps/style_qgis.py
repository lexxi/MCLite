"""Run in the QGIS 3.40 Python console. Styles loaded GeoPackage layers only."""
from qgis.core import (
    QgsProject, QgsVectorLayer, QgsProviderRegistry, QgsRuleBasedRenderer,
    QgsLineSymbol, QgsFillSymbol, QgsMarkerSymbol, QgsExpression,
    QgsPalLayerSettings, QgsVectorLayerSimpleLabeling, QgsTextFormat,
    QgsTextBufferSettings, QgsCoordinateReferenceSystem,
    QgsCoordinateTransform, QgsRectangle,
)
from qgis.PyQt.QtGui import QColor, QFont
from qgis.utils import iface

project = QgsProject.instance()

def find_layer(table):
    matches = []
    for layer in project.mapLayers().values():
        if not isinstance(layer, QgsVectorLayer) or not layer.isValid():
            continue
        uri = QgsProviderRegistry.instance().decodeUri(layer.providerType(), layer.source())
        if str(uri.get("path", "")).lower().endswith(".gpkg"):
            name = str(uri.get("layerName", ""))
            if name == table or (not name and layer.name().endswith(table)):
                matches.append(layer)
    if len(matches) != 1:
        raise RuntimeError("Expected exactly one GeoPackage layer: " + table)
    return matches[0]

layers = {name: find_layer(name) for name in ("points", "lines", "multipolygons")}

def tag(layer, key):
    fields = layer.fields().names()
    direct = '"' + key + '"' if key in fields else "NULL"
    if "other_tags" in fields:
        return "coalesce(" + direct + ', map_get(hstore_to_map("other_tags"), ' + "'" + key + "'), '')"
    return "coalesce(" + direct + ", '')"

def renderer(layer, specs):
    root = QgsRuleBasedRenderer.Rule(None)
    for title, expression, symbol in specs:
        parsed = QgsExpression(expression)
        if parsed.hasParserError():
            raise RuntimeError(title + ": " + parsed.parserErrorString())
        root.appendChild(QgsRuleBasedRenderer.Rule(symbol, 0, 0, expression, title))
    layer.setRenderer(QgsRuleBasedRenderer(root))
    layer.setLabelsEnabled(False)

def line(color, width):
    return QgsLineSymbol.createSimple({"line_color": color, "line_width": str(width),
                                      "line_width_unit": "MM"})

def fill(color):
    return QgsFillSymbol.createSimple({"color": color, "outline_style": "no"})

roads = layers["lines"]
h, w, rail = (tag(roads, k) for k in ("highway", "waterway", "railway"))
renderer(roads, [
    ("River / canal", w + " IN ('river','canal')", line("#76b6d5", .45)),
    ("Stream", w + " IN ('stream','ditch','drain') AND @map_scale < 100000", line("#8ac5df", .20)),
    ("Rail", rail + " IN ('rail','light_rail')", line("#666666", .24)),
    ("Motorway", h + " IN ('motorway','motorway_link')", line("#df8756", .85)),
    ("Main roads", h + " IN ('trunk','trunk_link','primary','primary_link')", line("#e9b75e", .65)),
    ("Secondary", h + " IN ('secondary','secondary_link')", line("#dfc684", .48)),
    ("Tertiary", h + " IN ('tertiary','tertiary_link')", line("#b3a798", .34)),
    ("Local roads", h + " IN ('residential','unclassified','living_street','service') AND @map_scale < 130000", line("#aaaaaa", .22)),
    ("Paths", h + " IN ('track','path','footway','cycleway','steps','bridleway') AND @map_scale < 65000", line("#b2a78e", .15)),
])

areas = layers["multipolygons"]
natural, landuse, water, waterway, building, leisure = (
    tag(areas, k) for k in ("natural", "landuse", "water", "waterway", "building", "leisure")
)
is_water = "(" + natural + " IN ('water','bay') OR " + water + " <> '' OR " + waterway + " = 'riverbank' OR " + landuse + " IN ('reservoir','basin'))"
renderer(areas, [
    ("Woodland", "(" + landuse + " = 'forest' OR " + natural + " = 'wood') AND NOT " + is_water, fill("#c4d5b3")),
    ("Grass / farmland", "(" + landuse + " IN ('grass','meadow','farmland','orchard','vineyard') OR " + natural + " IN ('grassland','scrub','heath') OR " + leisure + " IN ('park','garden')) AND NOT " + is_water, fill("#dce4c8")),
    ("Settlement", landuse + " IN ('residential','commercial','industrial','retail') AND NOT " + is_water, fill("#e5dfd6")),
    ("Rock / glacier", natural + " IN ('bare_rock','scree','glacier')", fill("#e2e2df")),
    ("Water", is_water, fill("#a7d2e8")),
    ("Buildings", building + " NOT IN ('','no') AND @map_scale < 45000", fill("#c2b7ac")),
])

places = layers["points"]
p = tag(places, "place")
# This is a project display filter, not a deletion from the GeoPackage.
if not places.setSubsetString('"name" IS NOT NULL AND "place" IN (\'city\',\'town\',\'village\',\'hamlet\')'):
    raise RuntimeError("Could not filter place layer")
marker = QgsMarkerSymbol.createSimple({"name": "circle", "color": "#58544e",
                                      "outline_style": "no", "size": "0.8"})
renderer(places, [("Places", "TRUE", marker)])
settings = QgsPalLayerSettings()
settings.isExpression = True
settings.fieldName = (
    'CASE WHEN ' + p + " IN ('city','town') OR (" + p +
    " = 'village' AND @map_scale < 220000) OR (" + p +
    " = 'hamlet' AND @map_scale < 65000) THEN \"name\" END"
)
fmt = QgsTextFormat()
fmt.setFont(QFont("Arial"))
fmt.setSize(9)
fmt.setColor(QColor("#302e2b"))
halo = QgsTextBufferSettings()
halo.setEnabled(True)
halo.setSize(.7)
halo.setColor(QColor("#fffdf7"))
fmt.setBuffer(halo)
settings.setFormat(fmt)
places.setLabeling(QgsVectorLayerSimpleLabeling(settings))
places.setLabelsEnabled(True)

root = project.layerTreeRoot()
others = [l for l in root.layerOrder() if l.id() not in {v.id() for v in layers.values()}]
root.setHasCustomLayerOrder(True)
root.setCustomLayerOrder([places, roads, areas] + others)
for layer in layers.values():
    root.findLayer(layer.id()).setItemVisibilityChecked(True)
    layer.triggerRepaint()

crs = QgsCoordinateReferenceSystem("EPSG:3857")
project.setCrs(crs)
project.setBackgroundColor(QColor("#f5f1e8"))
canvas = iface.mapCanvas()
canvas.setCanvasColor(QColor("#f5f1e8"))
bounds = QgsCoordinateTransform(QgsCoordinateReferenceSystem("EPSG:4326"), crs, project).transformBoundingBox(
    QgsRectangle(12.5, 46.3, 15.2, 47.3)
)
canvas.setExtent(bounds)
canvas.refresh()
print("Style applied. Source geometries unchanged. Save the project manually.")
print("The canvas view is bounded; tile export must use the same explicit extent.")
