##Selector modules

from ChargedHiggs.nanoAOD_processing.modules.triggerSelector import ChargedHiggsEleTriggerSelector
from ChargedHiggs.nanoAOD_processing.modules.electronSelector import ChargedHiggsElectronSelector
from ChargedHiggs.nanoAOD_processing.modules.bjetsSelector import ChargedHiggsBjetsSelector

##Producer modules

from ChargedHiggs.nanoAOD_processing.modules.smallHiggsProducer import ChargedHiggsSmallHiggsProducer
from ChargedHiggs.nanoAOD_processing.modules.WBosonProducer import ChargedHiggsWBosonProducer
from ChargedHiggs.nanoAOD_processing.modules.ChargedHiggsProducer import ChargedHiggsBosonProducer

module_list = [ 
                ChargedHiggsEleTriggerSelector(),
                ChargedHiggsElectronSelector(),
                ChargedHiggsBjetsSelector(),  
   
                ChargedHiggsSmallHiggsProducer(),    
                ChargedHiggsWBosonProducer(),   
                ChargedHiggsBosonProducer(),        
]
