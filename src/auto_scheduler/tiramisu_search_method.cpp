#include <tiramisu/auto_scheduler/search_method.h>
#include <random>

namespace tiramisu::auto_scheduler
{

void beam_search::search(syntax_tree& ast)
{
    if (ast.nb_explored_optims % NB_OPTIMIZATIONS == 0)
        ast.clear_new_optimizations();
       
    std::vector<syntax_tree*> children;
        
    // Look for an optimization that can be applied
    int nb_optims_tried = 0;
    int nb_explored_optims = ast.nb_explored_optims;
    
    while (children.size() == 0 && nb_optims_tried < NB_OPTIMIZATIONS && nb_explored_optims < max_depth)
    {
        optimization_type optim_type = DEFAULT_OPTIMIZATIONS_ORDER[nb_explored_optims % NB_OPTIMIZATIONS];
        
        children = scheds_gen->generate_schedules(ast, optim_type);
        
        nb_explored_optims++;
        nb_optims_tried++;
    }
      
    // Stop if no more optimizations can be applied
    if (children.size() == 0)
        return ;
       
    // Evaluate children and sort them from smallest to highest evaluation
    

   // evaluate while removing illegal versions
    auto iterator = children.begin();
    while (iterator != children.end())
    {
        (*iterator)->nb_explored_optims = nb_explored_optims;
        (*iterator)->transform_ast();
        
        if ((*iterator)->ast_is_legal() == false) {
            std::cout << "****if*************";
            // print deleted Ast 
            (*iterator)->print_previous_optims();
            std::cout << "\n-----------" << std::endl;
            (*iterator)->print_new_optims();
            (*iterator)->print_ast();
            (*iterator)->print_isl_states();
            std::cout << "\n<illegal>\n";
            delete (*iterator);
            iterator = children.erase(iterator);
        }
        else {
            std::cout << "*************else******";
            // evaluate and print Ast 
            (*iterator)->evaluation = eval_func->evaluate(*(*iterator));

            (*iterator)->print_previous_optims();
            std::cout << "\n-----------" << std::endl;
            (*iterator)->print_new_optims();
            (*iterator)->print_ast();
            std::cout << "Evaluation : " << (*iterator)->evaluation << std::endl << std::endl;
            (*iterator)->print_isl_states();
            (*iterator)->print_computations_accesses();
            std::cout << "\n<legal>\n";

            if ((*iterator)->evaluation < best_evaluation)
            {
                best_evaluation = (*iterator)->evaluation;
                best_ast = (*iterator);
            }

            ++iterator;

        }
        
        nb_explored_schedules++;
    }

    // Stop if we reached the maximum depth
    if (nb_explored_optims >= max_depth)
        return ;
        
    // Add the current AST to the list of children
    syntax_tree *ast_copy = ast.copy_ast();
    ast_copy->nb_explored_optims = nb_explored_optims;
    children.push_back(ast_copy);

    // Sort children from smallest evaluation to largest

    std::sort(children.begin(), children.end(), [](syntax_tree *a, syntax_tree *b) {
        return a->evaluation < b->evaluation;
    });

    // keep the top 'beam_size' children and delete the rest
    for (int i = beam_size; i < children.size(); ++i)
        delete children[i];

    children.resize(std::min(beam_size, (int)children.size()));
    
    // Search recursively on the best children
    for (syntax_tree *child : children)
    {
        child->search_depth = ast.search_depth + 1;        
        search(*child);
    }
}

void beam_search::search_save(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::default_random_engine rand_generator;

    if (ast.nb_explored_optims % NB_OPTIMIZATIONS == 0)
        ast.clear_new_optimizations();

    std::vector<syntax_tree*> children;

    // Look for an optimization that can be applied
    int nb_optims_tried = 0;
    int nb_explored_optims = ast.nb_explored_optims;

    while (children.size() == 0 && nb_optims_tried < NB_OPTIMIZATIONS && nb_explored_optims < max_depth)
    {
        optimization_type optim_type = DEFAULT_OPTIMIZATIONS_ORDER[nb_explored_optims % NB_OPTIMIZATIONS];
        children = scheds_gen->generate_schedules(ast, optim_type);

        nb_explored_optims++;
        nb_optims_tried++;
    }

    // Stop if no more optimizations can be applied
    if (children.size() == 0)
        return ;

    // Evaluate children and sort them from smallest to highest evaluation
    // evaluate while removing illegal versions
    auto iterator = children.begin();
    while (iterator != children.end())
    {
        syntax_tree *child = *iterator;
        child->nb_explored_optims = nb_explored_optims;
        child->transform_ast();

        if (child->schedule_is_prunable()){
            if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                // print deleted Ast
                child->print_previous_optims();
                std::cout << "\n-----------" << std::endl;
                child->print_new_optims();
                child->print_ast();
                std::cout << "\n<Schedule pruned>\n";
            }
            delete child;
            iterator = children.erase(iterator);
        }

        else if (!child->ast_is_legal()) {
            if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                // print deleted Ast
                child->print_previous_optims();
                std::cout << "\n-----------" << std::endl;
                child->print_new_optims();
                child->print_ast();
                child->print_isl_states();
                std::cout << "\n<illegal>\n";
            }
            delete child;
            iterator = children.erase(iterator);
        }
        else {

            // print and evaluate Ast

            if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                child->print_previous_optims();
                std::cout << "\n-----------" << std::endl;
                child->print_new_optims();
                child->print_ast();
                child->print_isl_states();
                std::cout << "\n<legal>\n";
                child->print_computations_accesses();
            }

            std::vector<float> measurements;
            if (child->can_set_default_evaluation()) { // if yes the child's evaluation is set to a default value
                measurements = {child->evaluation};
            }
            else{
                measurements = exec_eval->get_measurements(*child, false, schedule_timeout);
                child->evaluation = min_eval(measurements);
            }

            parent_trace->add_child_path(child, schedules_annotations->size());

            std::string schedule_annot = evaluate_by_learning_model::get_schedule_json(*child);

            //remove the last two characters }\n
            schedule_annot.pop_back();
            schedule_annot.pop_back();

            if (std::isfinite(child->evaluation)) // the evaluation is not finite mean that the schedule didn't run
                schedule_annot += ", \n\"execution_times\" : " + measurements_to_str(measurements) + "\n}\n";
            else
                schedule_annot += ", \n\"execution_times\" : null\n}\n";

            schedules_annotations->push_back(schedule_annot);

            if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                std::cout << "Schedule number "<< schedules_annotations->size() << std::endl;
                std::cout << "Evaluation : " << child->evaluation << std::endl;
                std::cout << "Number of measurements : " << measurements.size() << std::endl;
                std::cout << "===================================" << std::endl << std::endl;
            }

            if (std::isinf(child->evaluation))
                std::cerr<< "Evaluation of schedule "<< schedules_annotations->size() <<" failed "<< std::endl;

            if (child->evaluation < best_evaluation)
            {
                best_evaluation = child->evaluation;
                best_ast = child;
            }

            ++iterator;

        }

        nb_explored_schedules++;
    }

    // Stop if we reached the maximum depth
    if (nb_explored_optims >= max_depth)
        return ;

    // Add the current AST to the list of children
    syntax_tree *ast_copy = ast.copy_ast();
    ast_copy->nb_explored_optims = nb_explored_optims;
    children.push_back(ast_copy);
    parent_trace->add_child_path(ast_copy, parent_trace->get_candidate_id()); // keeps the same id since it's just copy

    // Sort children from smallest evaluation to largest
//    std::sort(children.begin(), children.end(), [](syntax_tree *a, syntax_tree *b) {
//        return a->evaluation < b->evaluation;
//    });
    // shuffle the children so that they are selected a random
    std::shuffle(std::begin(children), std::end(children), rand_generator);

    // keep the top 'beam_size' children and delete the rest
    for (int i = beam_size; i < children.size(); ++i)
        delete children[i];

    children.resize(std::min(beam_size, (int)children.size()));

    // Search recursively on the best children
    for (syntax_tree *child : children)
    {
        child->search_depth = ast.search_depth + 1;
        search_save(*child, schedules_annotations, parent_trace->child_mappings[child], schedule_timeout);
    }
}
std::vector < std::vector < std::vector<int> > > beam_search::get_random_matrcies(int nb_out_matrcies, int depth)
{
    std::vector <std::vector <  std::vector<int> >>  result(nb_out_matrcies);
    //result.at(0)= std::vector<int> (2); ERROR !!
    int nb_out_matrices = 5;
    int nb_valid_matrices = 0;
    int max_depth = 5;
    bool valid = false;
    while(nb_valid_matrices<nb_out_matrices)
    {
        while (!valid)
        {
            std::vector <  std::vector<int> >  random (depth);
            int i, o;
            srand(time(NULL));
            for(o = 0; o<depth; o++){
                random.at(o)= std::vector<int> (depth);
                for(i = 0; i<depth; i++)
                    random.at(o).at(i) = rand() % 14 - 7;;
            }
            // Check determinant equals 1
            bool det_bool = determinant(random, depth)==1;
            // Check upper right determinants equal 1
            if (!det_bool) continue;
            int k,l;
            int d=0,s=0;
            bool all_1 = true;
            for (k=1;k<depth;k++){
                    
                    std::vector <  std::vector<int> >  submatrixd(depth-k);
                    for(l=0;l<depth-k;l++){
                        submatrixd.at(l) = std::vector<int> (depth-k);
                    }
                    for (s=0;s<depth-k;s++){
                                for (d=0;d<depth-k;d++){
                                        submatrixd.at(s).at(d) = random.at(s+k).at(d+k); 
                                } 
                         }   
                    if(determinant(submatrixd, depth-k)!=1){ 
                        all_1 = false;
                        break;
                    }
            }
            valid = all_1;
        }
        nb_valid_matrices++;
    }
}
int determinant( std::vector <  std::vector<int> >  matrix, int n) {
   int det = 0;
   int i, o;
   std::vector <  std::vector<int> >  submatrix(n);
   for(o = 0; o<n; o++){
                submatrix.at(o)= std::vector<int> (n);}
   if (n == 2)
   return ((matrix.at(0).at(0) * matrix.at(1).at(1)) - (matrix.at(1).at(0) * matrix.at(0).at(1)));
   else {
      for (int x = 0; x < n; x++) {
         int subi = 0;
         for (int i = 1; i < n; i++) {
            int subj = 0;
            for (int j = 0; j < n; j++) {
               if (j == x)
               continue;
               submatrix.at(subi).at(subj) = matrix.at(i).at(j);
               subj++;
            }
            subi++;
         }
         det = det + (pow(-1, x) * matrix.at(0).at(x) * determinant( submatrix, n - 1 ));
      }
   }
   return det;
}
void beam_search::search_save_matrix(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::default_random_engine rand_generator;

    if (ast.nb_explored_optims % NB_OPTIMIZATIONS_MATRIX == 0)
        ast.clear_new_optimizations();

    std::vector<syntax_tree*> children;

    // Look for an optimization that can be applied
    int nb_optims_tried = 0;
    int nb_explored_optims = ast.nb_explored_optims;

    while (children.size() == 0 && nb_optims_tried < NB_OPTIMIZATIONS_MATRIX && nb_explored_optims < max_depth)
    {
        optimization_type optim_type = DEFAULT_OPTIMIZATIONS_ORDER_MATRIX[nb_explored_optims % NB_OPTIMIZATIONS_MATRIX];
        children = scheds_gen->generate_schedules(ast, optim_type);

        nb_explored_optims++;
        nb_optims_tried++;
    }

    // Stop if no more optimizations can be applied
    if (children.size() == 0)
        return ;

    // Evaluate children and sort them from smallest to highest evaluation
    // evaluate while removing illegal versions
    auto iterator = children.begin();
    std::vector < std::vector < std::vector<int> > > matrices;
    
    while (iterator != children.end())
    {
        int nb_matrices = 5;
        syntax_tree *child = *iterator;
        child->nb_explored_optims = nb_explored_optims;
        bool illegal = true;
        int shape = child->get_program_depth();
        matrices = get_random_matrcies(nb_matrices,shape);
        bool matrix = true;

        while(matrix && illegal && nb_matrices>0)
        {
            matrix = child->new_optims.back().type == MATRIX;
            child->transform_ast_matrix(matrices[nb_matrices]);
            nb_matrices--;
            if (child->schedule_is_prunable()){
                if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                    // print deleted Ast
                    child->print_previous_optims();
                    std::cout << "\n-----------" << std::endl;
                    child->print_new_optims();
                    child->print_ast();
                    std::cout << "\n<Schedule pruned>\n";
                }
                delete child;
                iterator = children.erase(iterator);
            }

            else if (!child->ast_is_legal()) {
                if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                    // print deleted Ast
                    child->print_previous_optims();
                    std::cout << "\n-----------" << std::endl;
                    child->print_new_optims();
                    child->print_ast();
                    child->print_isl_states();
                    std::cout << "\n<illegal>\n";
                }

                delete child;
                iterator = children.erase(iterator);
            }
            else {
                illegal = false;
                // print and evaluate Ast

                if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                    child->print_previous_optims();
                    std::cout << "\n-----------" << std::endl;
                    child->print_new_optims();
                    child->print_ast();
                    child->print_isl_states();
                    std::cout << "\n<legal>\n";
                    child->print_computations_accesses();
                }

                std::vector<float> measurements;
                if (child->can_set_default_evaluation()) { // if yes the child's evaluation is set to a default value
                    measurements = {child->evaluation};
                }
                else{
                    measurements = exec_eval->get_measurements(*child, false, schedule_timeout);
                    child->evaluation = min_eval(measurements);
                }

                parent_trace->add_child_path(child, schedules_annotations->size());

                std::string schedule_annot = evaluate_by_learning_model::get_schedule_json(*child);

                //remove the last two characters }\n
                schedule_annot.pop_back();
                schedule_annot.pop_back();

                if (std::isfinite(child->evaluation)) // the evaluation is not finite mean that the schedule didn't run
                    schedule_annot += ", \n\"execution_times\" : " + measurements_to_str(measurements) + "\n}\n";
                else
                    schedule_annot += ", \n\"execution_times\" : null\n}\n";

                schedules_annotations->push_back(schedule_annot);

                if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                    std::cout << "Schedule number "<< schedules_annotations->size() << std::endl;
                    std::cout << "Evaluation : " << child->evaluation << std::endl;
                    std::cout << "Number of measurements : " << measurements.size() << std::endl;
                    std::cout << "===================================" << std::endl << std::endl;
                }

                if (std::isinf(child->evaluation))
                    std::cerr<< "Evaluation of schedule "<< schedules_annotations->size() <<" failed "<< std::endl;

                if (child->evaluation < best_evaluation)
                {
                    best_evaluation = child->evaluation;
                    best_ast = child;
                }

                ++iterator;

            }

            nb_explored_schedules++;
        }
}

    // Stop if we reached the maximum depth
    if (nb_explored_optims >= max_depth)
        return ;

    // Add the current AST to the list of children
    syntax_tree *ast_copy = ast.copy_ast();
    ast_copy->nb_explored_optims = nb_explored_optims;
    children.push_back(ast_copy);
    parent_trace->add_child_path(ast_copy, parent_trace->get_candidate_id()); // keeps the same id since it's just copy

    // Sort children from smallest evaluation to largest
//    std::sort(children.begin(), children.end(), [](syntax_tree *a, syntax_tree *b) {
//        return a->evaluation < b->evaluation;
//    });
    // shuffle the children so that they are selected a random
    std::shuffle(std::begin(children), std::end(children), rand_generator);

    // keep the top 'beam_size' children and delete the rest
    for (int i = beam_size; i < children.size(); ++i)
        delete children[i];

    children.resize(std::min(beam_size, (int)children.size()));

    // Search recursively on the best children
    for (syntax_tree *child : children)
    {
        child->search_depth = ast.search_depth + 1;
        search_save_matrix(*child, schedules_annotations, parent_trace->child_mappings[child], schedule_timeout);
    }
}
void mcts::search(syntax_tree& ast)
{
    std::default_random_engine rand_generator;
    
    std::vector<syntax_tree*> samples;
    std::vector<syntax_tree*> children;
    std::vector<double> children_evals;
    
    for (int epoch = 0; epoch < nb_samples; ++epoch)
    {
        // Starting from the initial ast, generate optimizations until reaching max_depth
        syntax_tree *ast_sample = &ast;
        for (int depth = 0; depth < max_depth; ++depth)
        {
            optimization_type optim_type = DEFAULT_OPTIMIZATIONS_ORDER[depth % NB_OPTIMIZATIONS];
            children = scheds_gen->generate_schedules(*ast_sample, optim_type);
                        
            if (children.empty())
                continue;
                
            children_evals.clear();
            
            for (syntax_tree *child : children)
            {
                child->transform_ast();
                    
                child->evaluation = eval_func->evaluate(*child);
                children_evals.push_back(child->evaluation);
                
                nb_explored_schedules++;
            }
            
            // Add the current AST to the list of children
            children.push_back(ast_sample->copy_ast());
            children_evals.push_back(ast_sample->evaluation);
            
            // Sample an AST
            std::discrete_distribution<int> dist(children_evals.begin(), children_evals.end());
            ast_sample = children[dist(rand_generator)];
            
            samples.push_back(ast_sample);
        }
    }
    
    if (samples.empty())
        return ;
    
    // Sort schedules with respect to evaluations
    std::sort(samples.begin(), samples.end(), [](syntax_tree *a, syntax_tree *b) {
        return a->evaluation < b->evaluation;
    });
    
    // Execute top-k schedules and return the best
    for (int i = 0; i < topk; ++i)
    {
        float exec_time = exec_eval->evaluate(*samples[i]);
        if (exec_time < best_evaluation)
        {
            best_evaluation = exec_time;
            best_ast = samples[i];
        }
    }
}

void mcts::search_save(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::cerr<< "mcts::search_save not yet implemented" << std::endl;
    exit(1);
}
void mcts::search_save_matrix(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::cerr<< "mcts::search_save not yet implemented" << std::endl;
    exit(1);
}

// -------------------------------------------------------------------------- //

void beam_search_topk::search(syntax_tree& ast)
{
    // Do a beam search
    beam_search_subroutine(ast);
    
    // Sort schedules found
    std::sort(schedules.begin(), schedules.end(), [](syntax_tree *a, syntax_tree *b) {
        return a->evaluation < b->evaluation;
    });
    
    // Execute top-k schedules to find the best
    for (int i = 0; i < topk; ++i)
    {
        float exec_time = exec_eval->evaluate(*schedules[i]);
        if (exec_time < best_evaluation)
        {
            best_evaluation = exec_time;
            best_ast = schedules[i];
        }
    }
}

void beam_search_topk::search_save(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::cerr<< "beam_search_topk::search_save not yet implemented" << std::endl;
    exit(1);
}
void beam_search_topk::search_save_matrix(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::cerr<< "beam_search_topk::search_save not yet implemented" << std::endl;
    exit(1);
}

void beam_search_topk::beam_search_subroutine(syntax_tree& ast)
{
    if (ast.nb_explored_optims % NB_OPTIMIZATIONS == 0)
        ast.clear_new_optimizations();
       
    std::vector<syntax_tree*> children;
        
    // Look for an optimization that can be applied
    int nb_optims_tried = 0;
    int nb_explored_optims = ast.nb_explored_optims;
    
    while (children.size() == 0 && nb_optims_tried < NB_OPTIMIZATIONS && nb_explored_optims < max_depth)
    {
        optimization_type optim_type = DEFAULT_OPTIMIZATIONS_ORDER[nb_explored_optims % NB_OPTIMIZATIONS];
        children = scheds_gen->generate_schedules(ast, optim_type);
        
        nb_explored_optims++;
        nb_optims_tried++;
    }
       
    // Stop if no more optimizations can be applied
    if (children.size() == 0)
        return ;
       
    // Evaluate children and sort them from smallest to highest evaluation
    for (syntax_tree *child : children)
    {
        child->nb_explored_optims = nb_explored_optims;
        child->transform_ast();
            
        child->evaluation = eval_func->evaluate(*child);
        
        nb_explored_schedules++;
    }
        
    // Add the current AST to the list of children
    syntax_tree *ast_copy = ast.copy_ast();
    ast_copy->nb_explored_optims = nb_explored_optims;
    children.push_back(ast_copy);

    // Sort children from smallest evaluation to largest
    std::sort(children.begin(), children.end(), [](syntax_tree *a, syntax_tree *b) {
        return a->evaluation < b->evaluation;
    });
    
    for (int i = 0; i < beam_size; ++i)
        schedules.push_back(children[i]);
    
    // Stop if we reached the maximum depth
    if (nb_explored_optims >= max_depth)
        return ;

    // Search recursively on the best children
    for (int i = beam_size; i < children.size(); ++i)
        delete children[i];
        
    children.resize(std::min(beam_size, (int)children.size()));

    for (syntax_tree *child : children)
    {
        child->search_depth = ast.search_depth + 1;        
        search(*child);
    }
}

void beam_search_accuracy_evaluator::search(syntax_tree& ast)
{
    if (ast.nb_explored_optims % NB_OPTIMIZATIONS == 0)
        ast.clear_new_optimizations();
       
    std::vector<syntax_tree*> children;
        
    // Look for an optimization that can be applied
    int nb_optims_tried = 0;
    int nb_explored_optims = ast.nb_explored_optims;
    
    while (children.size() == 0 && nb_optims_tried < NB_OPTIMIZATIONS && nb_explored_optims < max_depth)
    {
        optimization_type optim_type = DEFAULT_OPTIMIZATIONS_ORDER[nb_explored_optims % NB_OPTIMIZATIONS];
        children = scheds_gen->generate_schedules(ast, optim_type);
        
        nb_explored_optims++;
        nb_optims_tried++;
    }
       
    // Stop if no more optimizations can be applied
    if (children.size() == 0)
        return ;
       
    // Evaluate children and sort them from smallest to highest evaluation
    for (syntax_tree *child : children)
    {
        child->nb_explored_optims = nb_explored_optims;
        child->transform_ast();
            
        child->evaluation = eval_func->evaluate(*child);
        
        // We evaluate both by the model and by execution
        model_evals_list.push_back(child->evaluation);
        exec_evals_list.push_back(exec_eval->evaluate(*child));
        
        if (child->evaluation < best_evaluation)
        {
            best_evaluation = child->evaluation;
            best_ast = child;
        }
        
        nb_explored_schedules++;
    }
    
    // Stop if we reached the maximum depth
    if (nb_explored_optims >= max_depth)
        return ;
        
    // Add the current AST to the list of children
    syntax_tree *ast_copy = ast.copy_ast();
    ast_copy->nb_explored_optims = nb_explored_optims;
    children.push_back(ast_copy);

    // Sort children from smallest evaluation to largest
    std::sort(children.begin(), children.end(), [](syntax_tree *a, syntax_tree *b) {
        return a->evaluation < b->evaluation;
    });

    // Search recursively on the best children
    for (int i = beam_size; i < children.size(); ++i)
        delete children[i];
        
    children.resize(std::min(beam_size, (int)children.size()));

    for (syntax_tree *child : children)
    {
        child->search_depth = ast.search_depth + 1;        
        search(*child);
    }
}

}
