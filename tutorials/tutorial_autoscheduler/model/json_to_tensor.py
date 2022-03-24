import numpy as np
import torch
import math
import copy

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
device = "cpu"
def get_representation_template(program_json):
    max_accesses = 15
    min_accesses = 1
    max_depth = 5 
    
    comp_name = list(program_json['computations'].keys())[0] # for single comp programs, there is only one computation
    comp_dict = program_json['computations'][comp_name] 
    
    
    
    comp_repr_template = []
#         iterators representation + transformations placeholders
    iterators_repr = []    
    for iter_i,iterator_name in enumerate(comp_dict['iterators']):
        iterator_dict = program_json['iterators'][iterator_name]
        iterators_repr.extend([iterator_dict['lower_bound'], iterator_dict['upper_bound']])
        
        # transformations placeholders
        l_code='L'+str(iter_i)
        iterators_repr.extend([#l_code+'Interchanged', 
                               #l_code+'Skewed', l_code+'SkewFactor', l_code+'SkewedExtent',
                               l_code+'Parallelized',
                               l_code+'Tiled', l_code+'TileFactor']) #unrolling is skipped since it is added only once
        
    # Adding padding
    iterator_repr_size = int(len(iterators_repr)/len(comp_dict['iterators']))
    iterators_repr.extend([0]*iterator_repr_size*(max_depth-len(comp_dict['iterators']))) # adding iterators padding 
    
    # Adding unrolling placeholder since unrolling can only be applied to the innermost loop 
    iterators_repr.extend(['Unrolled', 'UnrollFactor'])
    
    transformation_matrix_template = [0] * (len(comp_dict['iterators']) * len(comp_dict['iterators']))  
    iterators_repr.extend(transformation_matrix_template)
   
    #padding
    iterators_repr.extend([0]*(max_depth*max_depth-len(comp_dict['iterators'])*len(comp_dict['iterators'])))
    # Adding the iterators representation to computation vector       
    comp_repr_template.extend(iterators_repr)
    
    #  Write access representation to computation vector
    padded_write_matrix = pad_access_matrix(isl_to_write_matrix(comp_dict['write_access_relation']), max_depth)
    write_access_repr = [comp_dict['write_buffer_id']+1] + padded_write_matrix.flatten().tolist() # buffer_id + flattened access matrix 
#     print('write ', comp_dict['write_buffer_id']+1,'\n',padded_write_matrix)
    
    # Adding write access representation to computation vector
    comp_repr_template.extend(write_access_repr)
    
    # Read Access representation 
    read_accesses_repr=[]
    for read_access_dict in comp_dict['accesses']:
        read_access_matrix = pad_access_matrix(read_access_dict['access_matrix'], max_depth)
        read_access_repr = [read_access_dict['buffer_id']+1] + read_access_matrix.flatten().tolist() # buffer_id + flattened access matrix 
        read_accesses_repr.extend(read_access_repr)
#         print('read ', read_access_dict['buffer_id']+1,'\n',read_access_matrix)
        
    
    access_repr_len = (max_depth+1)*(max_depth + 2) + 1 # access matrix size +1 for buffer id
    read_accesses_repr.extend([0]*access_repr_len*(max_accesses-len(comp_dict['accesses']))) #adding accesses padding

    # Adding read Accesses to the representation to computation vector
    comp_repr_template.extend(read_accesses_repr)
    
    # Adding Operations count to computation vector
    comp_repr_template.append(comp_dict['number_of_additions'])
    comp_repr_template.append(comp_dict['number_of_subtraction'])
    comp_repr_template.append(comp_dict['number_of_multiplication'])
    comp_repr_template.append(comp_dict['number_of_division'])
    
    # Track the indices to the placeholders in a a dict
    placeholders_indices_dict = dict()
    for i, element in enumerate(comp_repr_template):
        if isinstance(element, str):
            placeholders_indices_dict[element] = i

    return comp_repr_template, placeholders_indices_dict
    
def pad_access_matrix(access_matrix, max_depth):
    access_matrix = np.array(access_matrix)
    access_matrix = np.c_[np.ones(access_matrix.shape[0]), access_matrix] # adding tags for marking the used rows
    access_matrix = np.r_[[np.ones(access_matrix.shape[1])], access_matrix] # adding tags for marking the used columns
    padded_access_matrix = np.zeros((max_depth + 1, max_depth + 2))
    padded_access_matrix[:access_matrix.shape[0],:access_matrix.shape[1]-1] = access_matrix[:,:-1] #adding padding to the access matrix before the last column
    padded_access_matrix[:access_matrix.shape[0],-1] = access_matrix[:,-1] #appending the last columns
    
    return padded_access_matrix

def isl_to_write_matrix(isl_map): # for now this function only support reductions
    comp_iterators_str = re.findall(r'\[(.*)\]\s*->', isl_map)[0]
    buffer_iterators_str = re.findall(r'->\s*\w*\[(.*)\]', isl_map)[0]
    buffer_iterators_str=re.sub(r"\w+'\s=","",buffer_iterators_str)
    comp_iter_names = re.findall(r'(?:\s*(\w+))+', comp_iterators_str)
    buf_iter_names = re.findall(r'(?:\s*(\w+))+', buffer_iterators_str)
    matrix = np.zeros([len(buf_iter_names),len(comp_iter_names)+1])
    for i,buf_iter in enumerate(buf_iter_names):
        for j,comp_iter in enumerate(comp_iter_names):
            if buf_iter==comp_iter:
                matrix[i,j]=1
                break
    return matrix

def isl_to_write_dims(isl_map): # return the buffer iterator that defines the write buffer
    buffer_iterators_str = re.findall(r'->\s*\w*\[(.*)\]', isl_map)[0]
    buffer_iterators_str = re.sub(r"\w+'\s=","",buffer_iterators_str)
    buf_iter_names = re.findall(r'(?:\s*(\w+))+', buffer_iterators_str)
    return buf_iter_names

def access_is_stencil(access):
    return np.any(access['access_matrix'], axis=0)[-1]
def get_datapoint_attributes(func_name, program_dict, schedule_index):

    schedule_json = program_dict['schedules_list'][schedule_index]
    sched_id = str(schedule_index).zfill(4)
        
    sched_str = sched_json_to_sched_str(schedule_json)
    exec_time = np.min(schedule_json['execution_times'])
    memory_use = program_dict['program_annotation']['memory_size']
    node_name = program_dict['node_name'] if 'node_name' in program_dict else 'unknown'

    return (func_name, sched_id, sched_str, exec_time, memory_use, node_name)


def get_schedule_representation(program_json, schedule_json, comp_repr_template, placeholders_indices_dict):
    comp_repr = comp_repr_template[:]
    comp_name = list(program_json['computations'].keys())[0] # for single comp programs, there is only one computation
    comp_dict =  program_json['computations'][comp_name]
    comp_schedule_dict=schedule_json[comp_name]
    #print("comp_dict")
    #print(comp_dict)
    #print("schedule_json")
    #print(schedule_json)
    for iter_i,iterator_name in enumerate(comp_dict['iterators']):
        l_code = 'L'+str(iter_i)
        
        # Interchange representation
        #interchanged = 0
        #if iterator_name in comp_schedule_dict['interchange_dims']:
        #    interchanged = 1 # interchanged = True
        #comp_repr[placeholders_indices_dict[l_code+'Interchanged']] = interchanged
        
        # Skewing representation
        #skewed = 0
        #skew_factor = 0
        #skew_extent = 0
        #if comp_schedule_dict['skewing'] and (iterator_name in comp_schedule_dict['skewing']['skewed_dims']):
        #    skewed = 1 #skewed: true
        #    skew_factor_index = comp_schedule_dict['skewing']['skewed_dims'].index(iterator_name)
        #    skew_factor = int(comp_schedule_dict['skewing']['skewing_factors'][skew_factor_index]) # skew factor
        #   skew_extent = int(comp_schedule_dict['skewing']['average_skewed_extents'][skew_factor_index]) # skew extent
        #comp_repr[placeholders_indices_dict[l_code+'Skewed']] = 0
        #comp_repr[placeholders_indices_dict[l_code+'SkewFactor']] = skew_factor
        #comp_repr[placeholders_indices_dict[l_code+'SkewedExtent']] = 0
        
        # Parallelization representation
        parallelized = 0
        if iterator_name == comp_schedule_dict['parallelized_dim']:
            parallelized = 1 # parallelized true
        comp_repr[placeholders_indices_dict[l_code+'Parallelized']] = parallelized
        
        # Tiling representation 
        tiled = 0
        tile_factor = 0
        if comp_schedule_dict['tiling'] and (iterator_name in comp_schedule_dict['tiling']['tiling_dims']):
            tiled = 1 #tiled: true
            tile_factor_index = comp_schedule_dict['tiling']['tiling_dims'].index(iterator_name)
            tile_factor = int(comp_schedule_dict['tiling']['tiling_factors'][tile_factor_index]) #tile factor
        comp_repr[placeholders_indices_dict[l_code+'Tiled']] = 0
        comp_repr[placeholders_indices_dict[l_code+'TileFactor']] = tile_factor
    
    # Unrolling Representation 
    unrolled = 0
    unroll_factor = 0
    if comp_schedule_dict['unrolling_factor']: #Unrolling is always aplied to the innermost loop 
        unrolled=1 #unrolled True
        unroll_factor = int(comp_schedule_dict['unrolling_factor']) #unroll factor
    comp_repr[placeholders_indices_dict['Unrolled']] = 0
    comp_repr[placeholders_indices_dict['UnrollFactor']] = unroll_factor
    # Matrix Representation
    start = placeholders_indices_dict['UnrollFactor'] + 1
    max_depth = 5
    
    padded_matrix = np.zeros((max_depth, max_depth))
    depth = len(comp_dict['iterators'])
    #print("transformation_matrix")
    #print (comp_schedule_dict['transformation_matrix'])
    # Adding padding so that the trandormations for each loop always start at the same index
    if(len(comp_schedule_dict['transformation_matrix'])>1):
        for i in range(max_depth):
            for j in range(max_depth):
                if (i<depth and j<depth ):
                    padded_matrix[i][j] = int(comp_schedule_dict['transformation_matrix'][(i*depth)+j])#+68
                
                
    padded_matrix = np.array(padded_matrix).flatten().tolist()
    for j in range( max_depth * max_depth ):
            comp_repr[start] = padded_matrix[j]
            start+=1

   
    
    #print("program json")
    #print(program_json)
    #print(comp_repr)
    return comp_repr
def sched_json_to_sched_str(sched_json): # Works only for 1 comp programs
    orig_loop_nest = []
    orig_loop_nest.append(sched_json['tree_structure']['loop_name'])
    child_list = sched_json['tree_structure']['child_list']
    while len(child_list)>0:
        child_loop = child_list[0]
        orig_loop_nest.append(child_loop['loop_name'])
        child_list = child_loop['child_list']
        
    comp_name = [n for n in sched_json.keys() if not n in ['unfuse_iterators','tree_structure','execution_times']][0]
    schedule = sched_json[comp_name]
    transf_loop_nest = orig_loop_nest
    sched_str = ''
    
    if schedule['interchange_dims']:
        first_dim_index = transf_loop_nest.index(schedule['interchange_dims'][0])
        second_dim_index = transf_loop_nest.index(schedule['interchange_dims'][1])
        sched_str+='I(L'+str(first_dim_index)+',L'+str(second_dim_index)+')'
        transf_loop_nest[first_dim_index], transf_loop_nest[second_dim_index] = transf_loop_nest[second_dim_index], transf_loop_nest[first_dim_index]
    if schedule['skewing']:
        first_dim_index = transf_loop_nest.index(schedule['skewing']['skewed_dims'][0])
        second_dim_index = transf_loop_nest.index(schedule['skewing']['skewed_dims'][1])
        first_factor = schedule['skewing']['skewing_factors'][0]
        second_factor = schedule['skewing']['skewing_factors'][1]
        sched_str+='S(L'+str(first_dim_index)+',L'+str(second_dim_index)+','+str(first_factor)+','+str(second_factor)+')'
    if schedule['parallelized_dim']:
        dim_index = transf_loop_nest.index(schedule['parallelized_dim'])
        sched_str+='P(L'+str(dim_index)+')'
    if schedule['tiling']:
        if schedule['tiling']['tiling_depth']==2:
            first_dim = schedule['tiling']['tiling_dims'][0]
            second_dim = schedule['tiling']['tiling_dims'][1]
            first_dim_index = transf_loop_nest.index(first_dim)
            second_dim_index = transf_loop_nest.index(second_dim)
            first_factor = schedule['tiling']['tiling_factors'][0]
            second_factor = schedule['tiling']['tiling_factors'][1]
            sched_str+='T2(L'+str(first_dim_index)+',L'+str(second_dim_index)+','+str(first_factor)+','+str(second_factor)+')'
            i = transf_loop_nest.index(first_dim)
            transf_loop_nest[i:i+1]=first_dim+'_outer', second_dim+'_outer'
            i = transf_loop_nest.index(second_dim)
            transf_loop_nest[i:i+1]=first_dim+'_inner', second_dim+'_inner'
        else: #tiling depth == 3
            first_dim = schedule['tiling']['tiling_dims'][0]
            second_dim = schedule['tiling']['tiling_dims'][1]
            third_dim = schedule['tiling']['tiling_dims'][2]
            first_dim_index = transf_loop_nest.index(first_dim)
            second_dim_index = transf_loop_nest.index(second_dim)
            third_dim_index = transf_loop_nest.index(third_dim)
            first_factor = schedule['tiling']['tiling_factors'][0]
            second_factor = schedule['tiling']['tiling_factors'][1]
            third_factor = schedule['tiling']['tiling_factors'][2]
            sched_str+='T3(L'+str(first_dim_index)+',L'+str(second_dim_index)+',L'+str(third_dim_index)+','+str(first_factor)+','+str(second_factor)+','+str(third_factor)+')'
            i = transf_loop_nest.index(first_dim)
            transf_loop_nest[i:i+1]=first_dim+'_outer', second_dim+'_outer', third_dim+'_outer'
            i = transf_loop_nest.index(second_dim)
            transf_loop_nest[i:i+1]=first_dim+'_inner', second_dim+'_inner', third_dim+'_inner'
            transf_loop_nest.remove(third_dim)
    if schedule['unrolling_factor']:
        dim_index = len(transf_loop_nest)-1
        dim_name =transf_loop_nest[-1]
        sched_str+='U(L'+str(dim_index)+','+schedule['unrolling_factor']+')'
        transf_loop_nest[dim_index:dim_index+1] = dim_name+'_Uouter', dim_name+'_Uinner'
    
    return sched_str
    
    
    
def update_exploration_trace_ids(node, func_predictions_df):
    if not node['schedule'] in list(func_predictions_df['sched_str']):
        return None
    sched = node['schedule']
    node['id'] = int(func_predictions_df.query('sched_str == @sched')['sched_name'])
    new_children = []
    for child in node['children']:
        updated = update_exploration_trace_ids(child, func_predictions_df)
        if updated != None:
            new_children.append(updated)
    node['children'] = new_children
    return node
  
def simulate_BeamSearch(root,beam_size,preds_dict=None,eval_mode='execution'): # given an exploration trace and a beam size, return the best candidate that can be found using that beam size
    children = root['children']
    if len(children)==0:
        return root
    if eval_mode == 'model':
        for child in children:
            child['prediction'] = preds_dict[str(child['id']).zfill(4)]
        root['prediction'] = preds_dict[str(root['id']).zfill(4)]
        children = sorted(children, key = lambda x: x['prediction'],reverse=True)
    elif eval_mode == 'execution':
        children = sorted(children, key = lambda x: x['evaluation'])
    children = children[:beam_size]
    bests = []
    for child in children:
        bests.append(simulate_BeamSearch(child,beam_size,preds_dict,eval_mode))
    bests.append(root) 
    if eval_mode == 'model':
        return max(bests, key = lambda x: x['prediction'])
    elif eval_mode == 'execution':
        return min(bests, key = lambda x: x['evaluation'])
        







def has_skippable_loop(prog_dict): # check if the program has a non time-step free iterator 
                                   # (has an iterator that is not used in accesses and the expression doesn't have reduction stentcils)
    
    program_json =  prog_dict['program_annotation']
    comp_name = list(program_json['computations'].keys())[0]
    comp_dict = program_json['computations'][comp_name]
    write_buffer_id = comp_dict['write_buffer_id']
    iterators = comp_dict['iterators']
    write_dims =  isl_to_write_dims(comp_dict['write_access_relation'])
    read_buffer_ids = [e['buffer_id'] for e in comp_dict['accesses']]
    
    
    if len(write_dims)==len(iterators): # if all loops used in write, no free loops
        # one special case of empty program
        if len(read_buffer_ids) == 1 and read_buffer_ids[0]==write_buffer_id and comp_dict['number_of_additions'] ==0 and comp_dict['number_of_subtraction'] ==0 and comp_dict['number_of_multiplication'] ==0 and comp_dict['number_of_division'] ==0: 
            return True
        return False
    
    if not write_buffer_id in read_buffer_ids: # if the calculation is clearly overwritten
        return True
    
    # find the simle reduction access
    found = False
    for access in comp_dict['accesses']:
        if access['buffer_id']==write_buffer_id and not access_is_stencil(access):
            found = True
            break
    if not found: # no simple reduction access is found, but we know that there is a reduction access in expression, so there is a skippable loop if the reduction is performed on last iterator, otherwise it's hardly skippable
        if write_dims[-1]!=iterators[-1]: # reduction is performed on the last iterator
            return True
    
    # find the non simple reduction accesses
    for access in comp_dict['accesses']:
        if access['buffer_id']==write_buffer_id and access_is_stencil(access): # a stencil access pattern is used
            return False
    
    # checking if there is a free loop (not used in write nor in read)
    read_dims_bools = []
    for access in comp_dict['accesses']: 
        read_dims_bools.append(np.any(access['access_matrix'], axis=0))
    read_dims_bools = np.any(read_dims_bools,axis=0)
    read_iterators = [iterators[i] for i, is_used in enumerate(read_dims_bools[:-1]) if is_used==True]
    used_iterators = set(write_dims+read_iterators)
    if len(used_iterators)==len(iterators): # all iterators are used in the computation
        return False
    
    if iterators[-1] in used_iterators: # the last iterator is not the dropped one, so the dropped loop shouldn't be skippable (knowing that there is a reduction access)
        if len(comp_dict['accesses'])>2:# has to have more than 2 accesses to make sure the loop isn't skippable, adding this condition for strictness
            return False
        
    return True