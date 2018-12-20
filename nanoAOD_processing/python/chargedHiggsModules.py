##Selector modules

from ChargedHiggs.nanoAOD_processing.modules.triggerSelector import cHiggsEleTriggerSelector
from ChargedHiggs.nanoAOD_processing.modules.electronSelector import cHiggsElectronSelector
from ChargedHiggs.nanoAOD_processing.modules.jetSelector import cHiggsJetSelector

##Producer modules

from ChargedHiggs.nanoAOD_processing.modules.MCWeightProducer import ChargedHiggsMCWeightProducer
from ChargedHiggs.nanoAOD_processing.modules.puWeightProducer import puAutoWeight


module_list = [ 
                cHiggsEleTriggerSelector(),
                cHiggsElectronSelector(2017, 36., 2.4),
                cHiggsJetSelector(2017, 30., 2.4),
   
                ChargedHiggsMCWeightProducer(),
                puAutoWeight(),
]
