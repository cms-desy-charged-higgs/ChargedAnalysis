##Selector modules

from ChargedHiggs.nanoAOD_processing.modules.triggerSelector import cHiggsEleTriggerSelector
from ChargedHiggs.nanoAOD_processing.modules.electronSelector import cHiggsElectronSelector
from ChargedHiggs.nanoAOD_processing.modules.jetSelector import cHiggsJetSelector
from ChargedHiggs.nanoAOD_processing.modules.metFilter import cMetFilter

##Producer modules

from ChargedHiggs.nanoAOD_processing.modules.quantitiesProducer import cHiggsQuantitiesProducer
from ChargedHiggs.nanoAOD_processing.modules.MCWeightProducer import ChargedHiggsMCWeightProducer
from ChargedHiggs.nanoAOD_processing.modules.puWeightProducer import puAutoWeight
from ChargedHiggs.nanoAOD_processing.modules.topPtWeightProducer import cHiggstopPtWeightProducer

module_list = [ 
                cMetFilter(2017),
                cHiggsEleTriggerSelector(),
                cHiggsElectronSelector(2017, 36., 2.4, 1),
                cHiggsJetSelector(2017, 30., 2.4, 4),
   
                cHiggsQuantitiesProducer(),
                ChargedHiggsMCWeightProducer(),
                puAutoWeight(),
                cHiggstopPtWeightProducer(),
]
