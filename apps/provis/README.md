Starting with the set of images one can acquire the sparse and dense reconstruction of the scene using SfM software, e.g. [VisualSfM](http://ccwu.me/vsfm/).

This readme will focus on the tested workflow with VisualSfM package.

Output of VisualSfM (should contain the following):

		model.nvm
		model.nvm.cmvs/

With the pypro classes the user is able to extract provenance data out of the sparse and dense points.
For VisualSfM this is achieved with `FormatSparseNVMV3` and `FormatDensePMVS` (subclasses of `FormatSparsePro` and `FormatDensePro` respectively).
If you want to extract data out of other datasets you have to implement your own classes.

```python3 ../pypro/main.py  <path to VisualSfM-output>/model.nvm <path to VisualSfM-output>/model.nvm.cmvs/00/models/option-0000.patch <path to VisualSfM-output>/00/models/option-0000.ply```

	output (located in the directory of call):
		sparse.prov
		sparse.prov.meta
		dense.prov
		dense.prov.meta

To collect provenance data (currently 6 floats: mean deviation, standard deviation, coefficient of variation of normals, as well as a copy of color channel data for debug purposes) from inside the LoD preprocessing pipeline, one should run the extended LoD preprocessing tool.

```./bin/lamure_preprocessing --files <path to VisualSfM-output>/00/models/option-0000.ply```

	output:
		model.0.bvh (in the same path as ply)
		model.0.lod (in the same path as ply)
		lod.meta (in the path of call)

Copy to <path to VisualSfM-output>/00/models and rename it to option-0000.prov.

Create the json file 'provenance_data_structure.json' with the following content to describe the types and associated visualizations of provenance data:

```
[{
	"type": "float",
	"visualization": "color"
}, {
	"type": "float",
	"visualization": "color"
}, {
	"type": "float",
	"visualization": "color"
}, {
	"type": "float",
	"visualization": "color"
}, {
	"type": "float",
	"visualization": "color"
}, {
	"type": "float",
	"visualization": "color"
}]
```

For the spatial allocation of data available per point one has to run the provenance test application (or implement the tree creation in the separate app):

```./bin/lamure_provenance -d <path to VisualSfM-output>/dense.prov -l <path to VisualSfM-output>/test.nvm.cmvs/00/models/option-0000.bvh -s <path to VisualSfM-output>/sparse.prov```
	
	output (located in the directory of call):
		tree.prov (required to be in the directory of call of the provis application)

Copy original images in to call folder of provis (the path has to remain consistent with the paths in the nvm file).

```./bin/lamure_provis -l <path to VisualSfM-output>/model.0.bvh -d <path to VisualSfM-output>/dense.prov  -s <path to VisualSfM-output>/sparse.prov -j <path to VisualSfM-output>/provenance_data_structure.json```