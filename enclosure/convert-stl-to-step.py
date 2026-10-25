"""Convert the printable STL parts of the enclosure into STEP solids, one file each.

OpenSCAD only exports meshes. This script sews every mesh into closed shells, turns
each shell into a solid, merges coplanar triangles back into single flat faces and
writes one STEP file per part (millimetres). It then reads every STEP file back and
checks that it holds valid solids with the same volume as the STL.

Needs OpenCascade for Python:  pip install cadquery-ocp
Usage (from the enclosure folder):  python convert-stl-to-step.py
"""
import struct
import sys
from pathlib import Path

from OCP.BRepBuilderAPI import BRepBuilderAPI_MakeSolid, BRepBuilderAPI_Sewing
from OCP.BRepCheck import BRepCheck_Analyzer
from OCP.BRepGProp import BRepGProp
from OCP.BRep import BRep_Builder
from OCP.GProp import GProp_GProps
from OCP.IFSelect import IFSelect_RetDone
from OCP.Interface import Interface_Static
from OCP.ShapeFix import ShapeFix_Solid
from OCP.ShapeUpgrade import ShapeUpgrade_UnifySameDomain
from OCP.STEPControl import STEPControl_AsIs, STEPControl_Reader, STEPControl_Writer
from OCP.StlAPI import StlAPI_Reader
from OCP.TopAbs import TopAbs_SHELL, TopAbs_SOLID
from OCP.TopExp import TopExp_Explorer
from OCP.TopoDS import TopoDS, TopoDS_Compound, TopoDS_Shape

# STL in stl/  ->  STEP in step/ (same base name)
PARTS = ["top-shell", "bottom-shell", "button-caps"]
SEW_TOLERANCE_MM = 1e-4


def stl_volume(path):
    """Volume of a closed binary STL mesh (divergence theorem), in mm^3."""
    data = path.read_bytes()
    count = struct.unpack("<I", data[80:84])[0]
    volume = 0.0
    for i in range(count):
        a, b, c = (struct.unpack("<3f", data[84 + i * 50 + 12 + k * 12:96 + i * 50 + 12 + k * 12])
                   for k in range(3))
        volume += (a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0])
                   + a[2] * (b[0] * c[1] - b[1] * c[0])) / 6.0
    return abs(volume)


def shape_volume(shape):
    props = GProp_GProps()
    BRepGProp.VolumeProperties_s(shape, props)
    return abs(props.Mass())


def count_solids(shape):
    explorer, count = TopExp_Explorer(shape, TopAbs_SOLID), 0
    while explorer.More():
        count += 1
        explorer.Next()
    return count


def mesh_to_solids(stl_path):
    """STL triangles -> sewn shells -> solids -> coplanar faces merged."""
    mesh = TopoDS_Shape()
    if not StlAPI_Reader().Read(mesh, str(stl_path)):
        raise RuntimeError(f"cannot read {stl_path}")
    sewing = BRepBuilderAPI_Sewing(SEW_TOLERANCE_MM)
    sewing.Add(mesh)
    sewing.Perform()

    compound, builder = TopoDS_Compound(), BRep_Builder()
    builder.MakeCompound(compound)
    explorer = TopExp_Explorer(sewing.SewedShape(), TopAbs_SHELL)
    while explorer.More():
        to_shell = getattr(TopoDS, "Shell", None) or TopoDS.Shell_s  # OCP 8 / OCP 7 name
        solid = BRepBuilderAPI_MakeSolid(to_shell(explorer.Current())).Solid()
        fixer = ShapeFix_Solid(solid)  # orients the shell outward
        fixer.Perform()
        builder.Add(compound, fixer.Solid())
        explorer.Next()

    unify = ShapeUpgrade_UnifySameDomain(compound, True, True, False)
    unify.Build()
    return unify.Shape()


def write_step(shape, step_path, name):
    Interface_Static.SetCVal_s("write.step.unit", "MM")
    Interface_Static.SetCVal_s("write.step.product.name", name)
    writer = STEPControl_Writer()
    writer.Transfer(shape, STEPControl_AsIs)
    if writer.Write(str(step_path)) != IFSelect_RetDone:
        raise RuntimeError(f"cannot write {step_path}")


def read_step(step_path):
    reader = STEPControl_Reader()
    if reader.ReadFile(str(step_path)) != IFSelect_RetDone:
        raise RuntimeError(f"cannot read back {step_path}")
    reader.TransferRoots()
    return reader.OneShape()


def main():
    root = Path(__file__).resolve().parent
    (root / "step").mkdir(exist_ok=True)
    ok = True
    for part in PARTS:
        stl_path, step_path = root / "stl" / f"{part}.stl", root / "step" / f"{part}.step"
        solids = mesh_to_solids(stl_path)
        write_step(solids, step_path, part)

        back = read_step(step_path)
        mesh_vol, step_vol = stl_volume(stl_path), shape_volume(back)
        valid = BRepCheck_Analyzer(back).IsValid()
        same = abs(step_vol - mesh_vol) <= 1e-3 * mesh_vol
        ok &= valid and same
        print(f"{part:13s} solids={count_solids(back):2d} valid={valid} "
              f"volume STL={mesh_vol:9.1f} STEP={step_vol:9.1f} mm3 "
              f"size={step_path.stat().st_size / 1e6:.1f} MB")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
