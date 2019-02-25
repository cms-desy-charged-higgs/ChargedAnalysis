##Selector modules

from ChargedHiggs.nanoAOD_processing.modules.triggerSelector import cHiggsTriggerSelector
from ChargedHiggs.nanoAOD_processing.modules.electronSelector import cHiggsElectronSelector
from ChargedHiggs.nanoAOD_processing.modules.muonSelector import cHiggsMuonSelector
from ChargedHiggs.nanoAOD_processing.modules.jetSelector import cHiggsJetSelector
from ChargedHiggs.nanoAOD_processing.modules.metFilter import cHiggsMetFilter

##Producer modules

from ChargedHiggs.nanoAOD_processing.modules.nGenProducer import cHiggsnGenProducer
from ChargedHiggs.nanoAOD_processing.modules.quantitiesProducer import cHiggsQuantitiesProducer
from ChargedHiggs.nanoAOD_processing.modules.MCWeightProducer import cHiggsMCWeightProducer
from ChargedHiggs.nanoAOD_processing.modules.puWeightProducer import cHiggspuWeightProducer
from ChargedHiggs.nanoAOD_processing.modules.topPtWeightProducer import cHiggstopPtWeightProducer

module_list = {
            "e4b": [ 
                    cHiggsnGenProducer(),

                    cHiggsMetFilter(2017),
                    cHiggsTriggerSelector(["HLT_Ele35_WPTight_Gsf"]),
                    cHiggsElectronSelector(2017, 38., 2.4, 1, 2),
                    cHiggsMuonSelector(2017, 20., 2.4, 0),
                    cHiggsJetSelector(2017, 30., 2.4, 4),
   
                    cHiggsQuantitiesProducer(),
                    cHiggsMCWeightProducer(2017),
                    cHiggspuWeightProducer(2017),
                    cHiggstopPtWeightProducer(),
            ],

            "m4b": [ 
                    cHiggsnGenProducer(),

                    cHiggsMetFilter(2017),
                    cHiggsTriggerSelector(["HLT_IsoMu27"]),
                    cHiggsMuonSelector(2017, 30., 2.4, 1, 2),
                    cHiggsElectronSelector(2017, 20., 2.4, 0),
                    cHiggsJetSelector(2017, 30., 2.4, 4),
   
                    cHiggsQuantitiesProducer(),
                    cHiggsMCWeightProducer(2017),
                    cHiggspuWeightProducer(2017),
                    cHiggstopPtWeightProducer(),
            ],
}
