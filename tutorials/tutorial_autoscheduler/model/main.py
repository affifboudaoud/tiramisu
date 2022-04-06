from os import environ
import sys, json
import torch
from feed_forward import Model_FeedForward
from json_to_tensor import *

import warnings
warnings.filterwarnings('ignore', category=DeprecationWarning) 
warnings.filterwarnings('ignore', category=UserWarning)

model_path = './temps_save_400K_single_comp_4Layers.pkl'

with torch.no_grad():
    device = 'cpu'
    torch.device('cpu')

    environ['layers'] = '350 400 200 180'
    environ['dropouts'] = '0.175 ' * 4

    input_size = 755
    output_size = 1

    layers_sizes = list(map(int, environ.get('layers', '350 400 200 180').split()))
    drops = list(map(float, environ.get('dropouts', '0.175 0.175 0.175 0.175').split()))

    model = Model_FeedForward(input_size,layer_sizes= [ 350, 400, 200, 180], drops=[0.175, 0.175, 0.175,0.175])
    model.load_state_dict(torch.load(model_path, map_location='cpu'))
    model.to(device)
    model.eval()

    try:
        while True:
            prog_json = input()
            sched_json = input()

            prog_json = json.loads(prog_json)
            sched_json = json.loads(sched_json)
	    
            comp_repr_template, placeholders_indices_dict = get_representation_template(prog_json)	

            schedule_vector = get_schedule_representation(prog_json, sched_json, comp_repr_template, placeholders_indices_dict)
            
            batched_schedule_vector = torch.FloatTensor(schedule_vector)

            batched_schedule_vector = batched_schedule_vector[None, :]
            
            speedup = model.forward(batched_schedule_vector)
            print(float(speedup.item()))
            
    except EOFError:
        exit()
        
