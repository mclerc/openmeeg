# set paths
OPENMEEGDATAPATH=../..
OPENMEEGBINPATH=../../../src

# general settings
HEADMODELDIR=Head3
COMPUTATIONDIR=Head3
INTERMEDIATEDIR=Head3
#TYPE=EEG-dip
TYPE=MEG-dip

# forward problem settings
EEGNOISELEVEL=0.00
MEGNOISELEVEL=0.00

# inverse problem settings
EEGDATAWEIGHT=1
MEGDATAWEIGHT=1
SMOOTHWEIGHT=1e-4
SMOOTHTYPE=HEAT
STOPPINGTOL=0
MAXNBITER=10000
# provided files# provided files
SOURCEMESH=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).tri
SOURCEPOINTS=$(OPENMEEGDATAPATH)/Computations/$(HEADMODELDIR)/$(HEADMODELDIR).dip
GEOMETRY=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).geom
CONDUCTIVITY=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).cond
PATCHES=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).patches
SQUIDS=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR)-noradial.squids
# SQUIDS=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).squids
REALSOURCESDATA=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).src
REALSOURCESDATA-DIP=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).srcdip
REALMEGDATA=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).meg
REALEEGDATA=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).eeg

# computed files
HEADMATRIX=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).hm
HEADMATRIXINV=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).hm_inv
SURFSOURCEMATRIX=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).ssm
DIPSOURCEMATRIX=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).dsm
HEAD2EEGMATRIX=$(OPENMEEGDATAPATH)/IntermediateFiles/$(INTERMEDIATEDIR)/$(INTERMEDIATEDIR).h2em
SURFSOURCE2MEGMATRIX=$(OPENMEEGDATAPATH)/IntermediateFiles/$(INTERMEDIATEDIR)/$(INTERMEDIATEDIR).ss2mm
HEAD2MEGMATRIX=$(OPENMEEGDATAPATH)/IntermediateFiles/$(INTERMEDIATEDIR)/$(INTERMEDIATEDIR).h2mm
EEGGAINMATRIX=$(OPENMEEGDATAPATH)/IntermediateFiles/$(INTERMEDIATEDIR)/$(INTERMEDIATEDIR).sgem
MEGGAINMATRIX=$(OPENMEEGDATAPATH)/IntermediateFiles/$(INTERMEDIATEDIR)/$(INTERMEDIATEDIR).sgmm

EEGGAINMATRIX-DIP=$(OPENMEEGDATAPATH)/IntermediateFiles/$(INTERMEDIATEDIR)/$(INTERMEDIATEDIR).dgem
MEGGAINMATRIX-DIP=$(OPENMEEGDATAPATH)/IntermediateFiles/$(INTERMEDIATEDIR)/$(INTERMEDIATEDIR).dgmm
DIPSOURCE2MEGMATRIX=$(OPENMEEGDATAPATH)/IntermediateFiles/$(INTERMEDIATEDIR)/$(INTERMEDIATEDIR).ds2mm

AIVECTOR=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).ai
SMOOTHMATRIX=$(OPENMEEGDATAPATH)/Models/$(HEADMODELDIR)/$(HEADMODELDIR).smooth
ESTIMATEDSOURCESDATA=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).est_src
ESTIMATEDMEGDATA=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).est_meg
ESTIMATEDEEGDATA=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR).est_eeg

ESTIMATEDEEGDATA-DIP=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR)-point.est_eeg
ESTIMATEDMEGDATA-DIP=$(OPENMEEGDATAPATH)/Computations/$(COMPUTATIONDIR)/$(COMPUTATIONDIR)-point.est_meg

# symbolic targets
ifeq "$(TYPE)"  "MEG"
assemble: $(MEGGAINMATRIX)
endif

ifeq "$(TYPE)"  "EEG"
assemble: $(EEGGAINMATRIX)
endif

ifeq "$(TYPE)"  "MEG"
forward: $(ESTIMATEDMEGDATA)
endif

ifeq "$(TYPE)"  "EEG"
forward: $(ESTIMATEDEEGDATA)
endif

ifeq "$(TYPE)"  "MEG-dip"
forward: $(ESTIMATEDMEGDATA-DIP)
endif

ifeq "$(TYPE)"  "EEG-dip"
forward: $(ESTIMATEDEEGDATA-DIP)
endif

inverse: $(ESTIMATEDSOURCESDATA)

ifeq "$(TYPE)"  "MEG"
forinv:
	$(MAKE) forward
	\cp $(ESTIMATEDMEGDATA) $(REALMEGDATA)
	$(MAKE) inverse
endif

ifeq "$(TYPE)"  "EEG"
forinv:
	$(MAKE) forward
	\cp $(ESTIMATEDEEGDATA) $(REALEEGDATA)
	$(MAKE) inverse
endif

clean:
	rm -f $(HEADMATRIX) $(HEADMATRIXINV) $(SURFSOURCEMATRIX) $(HEAD2EEGMATRIX) $(SURFSOURCE2MEGMATRIX) $(HEAD2MEGMATRIX) $(EEGGAINMATRIX) $(EEGGAINMATRIX) $(MEGGAINMATRIX) $(AIVECTOR) $(SMOOTHMATRIX) $(ESTIMATEDSOURCESDATA) $(ESTIMATEDMEGDATA) $(ESTIMATEDEEGDATA) $(SURFSOURCEMATRIX1) $(SURFSOURCEMATRIX2) $(SURFSOURCEMATRIX3) $(MEGGAINMATRIX1) $(MEGGAINMATRIX2) $(MEGGAINMATRIX3) $(SURFSOURCE2MEGMATRIX1) $(SURFSOURCE2MEGMATRIX2) $(SURFSOURCE2MEGMATRIX3)

$(SURFSOURCEMATRIX) :  $(GEOMETRY) $(CONDUCTIVITY) $(SOURCEMESH)
	"$(OPENMEEGBINPATH)/om_assemble" -SurfSourceMat $(GEOMETRY) $(CONDUCTIVITY) $(SOURCEMESH) $(SURFSOURCEMATRIX)

$(DIPSOURCEMATRIX) :  $(GEOMETRY) $(SOURCEPOINTS)
	"$(OPENMEEGBINPATH)/om_assemble" -DipSourceMat $(GEOMETRY) $(CONDUCTIVITY) $(SOURCEPOINTS) $(DIPSOURCEMATRIX)

$(HEADMATRIX) : $(GEOMETRY) $(CONDUCTIVITY)
	"$(OPENMEEGBINPATH)/om_assemble" -HeadMat $(GEOMETRY) $(CONDUCTIVITY) $(HEADMATRIX)

$(HEAD2EEGMATRIX) :  $(GEOMETRY) $(CONDUCTIVITY) $(PATCHES)
	"$(OPENMEEGBINPATH)/om_assemble" -Head2EEGMat $(GEOMETRY) $(CONDUCTIVITY) $(PATCHES) $(HEAD2EEGMATRIX)

$(HEAD2MEGMATRIX) :  $(GEOMETRY) $(CONDUCTIVITY) $(SQUIDS)
	"$(OPENMEEGBINPATH)/om_assemble" -Head2MEGMat $(GEOMETRY) $(CONDUCTIVITY) $(SQUIDS) $(HEAD2MEGMATRIX)

$(SURFSOURCE2MEGMATRIX) :  $(SOURCEMESH) $(SQUIDS)
	"$(OPENMEEGBINPATH)/om_assemble" -SurfSource2MEGMat $(SOURCEMESH) $(SQUIDS) $(SURFSOURCE2MEGMATRIX)

$(DIPSOURCE2MEGMATRIX) : $(SOURCEPOINTS) $(SQUIDS)
	"$(OPENMEEGBINPATH)/om_assemble" -DipSource2MEGMat $(SOURCEPOINTS) $(SQUIDS) $(DIPSOURCE2MEGMATRIX)

$(SURFSOURCE2MEGMATRIX2) :  $(SOURCEMESH) $(SOURCEMESH2) $(SQUIDS)

$(ESTIMATEDEEGDATA) : $(EEGGAINMATRIX) $(REALSOURCESDATA)
	"$(OPENMEEGBINPATH)/om_forward" $(EEGGAINMATRIX)  $(REALSOURCESDATA)  $(ESTIMATEDEEGDATA)  $(EEGNOISELEVEL)

$(ESTIMATEDMEGDATA) : $(MEGGAINMATRIX) $(REALSOURCESDATA)
	"$(OPENMEEGBINPATH)/om_forward" $(MEGGAINMATRIX)  $(REALSOURCESDATA)  $(ESTIMATEDMEGDATA)  $(MEGNOISELEVEL)

$(ESTIMATEDEEGDATA-DIP) : $(EEGGAINMATRIX-DIP) $(REALSOURCESDATA-DIP)
	"$(OPENMEEGBINPATH)/om_forward" $(EEGGAINMATRIX-DIP)  $(REALSOURCESDATA-DIP)  $(ESTIMATEDEEGDATA-DIP)  $(EEGNOISELEVEL)

$(ESTIMATEDMEGDATA-DIP) : $(MEGGAINMATRIX-DIP) $(REALSOURCESDATA-DIP)
	"$(OPENMEEGBINPATH)/om_forward" $(MEGGAINMATRIX-DIP)  $(REALSOURCESDATA-DIP)  $(ESTIMATEDMEGDATA-DIP)  $(MEGNOISELEVEL)

$(EEGGAINMATRIX) : $(HEADMATRIXINV) $(SURFSOURCEMATRIX) $(HEAD2EEGMATRIX)
	"$(OPENMEEGBINPATH)/om_gain" -EEG $(HEADMATRIXINV)  $(SURFSOURCEMATRIX)  $(HEAD2EEGMATRIX)  $(EEGGAINMATRIX)

$(MEGGAINMATRIX) : $(HEADMATRIXINV) $(SURFSOURCEMATRIX) $(HEAD2MEGMATRIX) $(SURFSOURCE2MEGMATRIX)
	"$(OPENMEEGBINPATH)/om_gain" -MEG $(HEADMATRIXINV)  $(SURFSOURCEMATRIX)  $(HEAD2MEGMATRIX)  $(SURFSOURCE2MEGMATRIX)  $(MEGGAINMATRIX)

$(EEGGAINMATRIX-DIP) : $(HEADMATRIXINV) $(DIPSOURCEMATRIX) $(HEAD2EEGMATRIX)
	"$(OPENMEEGBINPATH)/om_gain" -EEG $(HEADMATRIXINV)  $(DIPSOURCEMATRIX)  $(HEAD2EEGMATRIX)  $(EEGGAINMATRIX-DIP)

$(MEGGAINMATRIX-DIP) : $(HEADMATRIXINV) $(DIPSOURCEMATRIX) $(HEAD2MEGMATRIX) $(DIPSOURCE2MEGMATRIX)
	"$(OPENMEEGBINPATH)/om_gain" -MEG $(HEADMATRIXINV)  $(DIPSOURCEMATRIX)  $(HEAD2MEGMATRIX)  $(DIPSOURCE2MEGMATRIX)  $(MEGGAINMATRIX-DIP)

ifeq "$(TYPE)"  "EEG"
$(ESTIMATEDSOURCESDATA) : $(EEGGAINMATRIX) $(SMOOTHMATRIX) $(REALEEGDATA)
	"$(OPENMEEGBINPATH)/om_inverse" $(EEGGAINMATRIX)  $(SMOOTHMATRIX)  $(AIVECTOR)  $(REALEEGDATA)  $(ESTIMATEDSOURCESDATA)   $(SMOOTHWEIGHT)  $(SMOOTHTYPE)  $(MAXNBITER)  $(STOPPINGTOL)
endif

ifeq "$(TYPE)"  "MEG"
$(ESTIMATEDSOURCESDATA) : $(MEGGAINMATRIX) $(SMOOTHMATRIX) $(REALMEGDATA)
	"$(OPENMEEGBINPATH)/om_inverse" $(MEGGAINMATRIX)  $(SMOOTHMATRIX)  $(AIVECTOR)  $(REALMEGDATA)  $(ESTIMATEDSOURCESDATA)   $(SMOOTHWEIGHT)  $(SMOOTHTYPE)  $(MAXNBITER)  $(STOPPINGTOL)
endif

$(HEADMATRIXINV) : $(HEADMATRIX)
	"$(OPENMEEGBINPATH)/om_minverser"  $(HEADMATRIX)  $(HEADMATRIXINV)

$(SMOOTHMATRIX) : $(SOURCEMESH)
	"$(OPENMEEGBINPATH)/om_presmooth"  $(SOURCEMESH)  $(SMOOTHMATRIX)  $(AIVECTOR)

