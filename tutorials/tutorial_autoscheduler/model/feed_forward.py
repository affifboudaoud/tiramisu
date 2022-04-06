
from tqdm.notebook import tqdm
import re
import torch
import numpy as np
import random 
import torch.nn as nn
from torch import optim
import time
import sys
import pickle

class Model_FeedForward(nn.Module):
    def __init__(self, input_size, layer_sizes=[600, 350, 200, 180], drops=[0.225, 0.225, 0.225, 0.225], output_size=1):
        super().__init__()
        regression_layer_sizes = [input_size] + layer_sizes
        self.regression_layers = nn.ModuleList()
        self.regression_dropouts= nn.ModuleList()
        for i in range(len(regression_layer_sizes)-1):
            self.regression_layers.append(nn.Linear(regression_layer_sizes[i], regression_layer_sizes[i+1], bias=True))
#             nn.init.xavier_uniform_(self.regression_layers[i].weight)
            nn.init.sparse_(self.regression_layers[i].weight, 0.1)
            self.regression_dropouts.append(nn.Dropout(drops[i]))
        self.predict = nn.Linear(regression_layer_sizes[-1], output_size, bias=True)
#         nn.init.xavier_uniform_(self.predict.weight)
        nn.init.sparse_(self.predict.weight, 0.1)
        self.ELU=nn.ELU()

    def forward(self, batched_X): 
        x = batched_X
        for i in range(len(self.regression_layers)):
            x = self.regression_layers[i](x)
            x = self.regression_dropouts[i](self.ELU(x))
        out = self.predict(x)
            
        return self.ELU(out[:,0])
