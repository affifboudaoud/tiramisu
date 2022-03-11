#include <sys/wait.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include <random>
#include <functional>
#include <exception>

#include <stdexcept>
#define TIME_LIMIT 1000

// exception for Unrolling when scheduled program has an IF statement
struct UnrollingException : public std::exception {
    const char * what () const throw ()
        {
            return "unrolling error : unrolled loop level is a user node due to dimension error";
        }
};

namespace tiramisu::auto_scheduler
{
 std::string get_name_ast_expr_isl( isl_ast_expr *expr);
  std::string get_expr_isl_string( isl_ast_expr *expr, bool is_bound);
 int get_value(isl_ast_expr *expr,std::vector<std::vector<int>> isl_ast_map );

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

int timeout = 0;
int child_done = 0;
int cont = 0;
  void child_handler(int sig)
{
    child_done = 1;
}
int nb_exec = std::stoi(std::getenv("MAX_RUNS"));
void alarm_handler(int sig)
{
    timeout = 1;
}
void sig_usr(int signo){
    cont = 1; 
    
}


void beam_search::search_save(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::default_random_engine rand_generator;
    
    

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

        if (!child->ast_is_legal()) {
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
            int fd[2];
            
            // create pipe descriptors
	        pipe(fd);
            timeout=0;
            child_done=0;
            cont = false;
            std::vector<float> measurements;
            
            // create a child process to execute the code and get measurements
            // this is added so that we can limit the code generation time for large programs
            pid_t pid;
            pid = fork();

            if (pid == -1) {
                perror("fork failed");
                exit(1);
            } else if (pid == 0) {
                // put get_measurements in a try block in the case of erroneous unrolling
                // this should be added to ast_is_legal but for now it can only be detected at apply_optimization level
                try{
                     measurements = exec_eval->get_measurements_matrix(*child, false, schedule_timeout);
                }
                catch(UnrollingException e){ 
                     // Remove all the optimizations
                    exec_eval->fct->reset_schedules();
                    measurements.clear();
                    measurements.push_back(std::numeric_limits<float>::infinity());
                    cont = 1;
                }
                int size =measurements.size();
                float ar[measurements.size()];
                // put the measurements in an array to be sent to the parent process
                for(int i=0;i<measurements.size();i++) ar[i]=measurements.at(i);
                close(fd[0]);
                // send number of measurements to parent process
                write(fd[1], &size, sizeof(size));
                // send measurements to parent process
                write(fd[1], &ar, sizeof(ar));
                // close the pipe
                close(fd[1]);
                // end child process
                _exit(1);
            }

            // set up the signal handlers after forking so the child doesn't inherit them
            // an alarm in the case of the timelimit is reached and the child process was interrupted
            signal(SIGALRM, alarm_handler);
            // an alarm in the case of the child process ending ie. both data generation and evaluation are done
            signal(SIGCHLD, child_handler);
            // an alarm in the case of the data generation ending before the timelimit. We still need to wait for the evaluation to be done. 
            signal(SIGUSR1,sig_usr);
            // install an alarm to be fired after TIME_LIMIT
            
            // set the alarm
            alarm(TIME_LIMIT);
            
            pause();

            if (timeout) {
                
                // if the timeout has been reached    
                int result = waitpid(pid, NULL, WNOHANG);
                if (result == 0) {
                    // child still running, so kill it
                    
                    // remove all the optimizations
                    exec_eval->fct->reset_schedules();
                    measurements.clear();
                    // if the timeout has been reached, put infinity as a measurement
                    measurements.push_back(std::numeric_limits<float>::infinity());
                    // cancel any previously set alarm 
                    alarm(0); 
                    // kill child process
                    kill(pid, 9);
                    
                
                    waitpid(-1,NULL,0);
                } else {
                    // if by the time we detect that the alarm has been raised, the evaluation has been completed
                    // we recieve the measurements from the child 
                    int size = 0;
                    close(fd[1]);
                    read(fd[0], &size, sizeof(int));
                    float ar[size];
                    read(fd[0], &ar, size*sizeof(float));
                    for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                    close(fd[0]);
                    
                }
                   
            }else if (child_done) {
                // the execution of the child is done, both code generation and evaluation
                // we recieve the measurements from the child
                int size =0;
                close(fd[1]);
                read(fd[0], &size, sizeof(int));
                float ar[size];
                read(fd[0], &ar, size*sizeof(float));
                for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                close(fd[0]);
                
                waitpid(-1,NULL,0);
            }else if(cont){
                // execution is not done but the code generation is done. This happens for large programs that take a long time to execute
                // cancel the timeout alarm 
                alarm(0);
                int size=0;
                // we wait for the evalution of the generated code
                while(!child_done){}
                // after evealuation is done, we recieve the measurements from the child
                close(fd[1]);
                read(fd[0], &size, sizeof(int));
                float ar[size];
                read(fd[0], &ar, size*sizeof(float));
                for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                close(fd[0]);
                waitpid(-1,NULL,0);
            }
            
           
            
            child->evaluation = min_eval(measurements);
            
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
    std::sort(children.begin(), children.end(), [](syntax_tree *a, syntax_tree *b) {
        return a->evaluation < b->evaluation;
    });
    // shuffle the children so that they are selected a random
//    std::shuffle(std::begin(children), std::end(children), rand_generator);
    
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
/*
multiply two matrices AxB
*/
std::vector<std::vector<int>>  multiply(const std::vector<std::vector<int>> & m1, const std::vector<std::vector<int>> & m2)
{
std::vector<std::vector<int>> result(m1.size(), std::vector<int>(m2.at(0).size()));

    for(std::size_t row = 0; row < result.size(); ++row) {
        for(std::size_t col = 0; col < result.at(0).size(); ++col) {
            for(std::size_t inner = 0; inner < m2.size(); ++inner) {
                result.at(row).at(col) += m1.at(row).at(inner) * m2.at(inner).at(col);
            }
        }
    }
    return result;
}


/*
check if a matrix is the identity matrix
*/
bool is_identity(std::vector < std::vector<int> > matrix){
    for(int l = 0; l<matrix.size(); l++){
            for(int c = 0; c<matrix.size(); c++){
                            if (l!=c && matrix.at(l).at(c)!=0){
                                return false;
                            }else{
                                if(matrix.at(l).at(c)!=1){
                                    return false;
                                }
                            }
            }
        }
        return true;
}
/*
return identity matrix
*/
std::vector <  std::vector<int> > get_identity(int depth){
    std::vector <  std::vector<int> >  matrix(depth);
        for(int l = 0; l<matrix.size(); l++){
            matrix.at(l)= std::vector<int>(depth);
            for(int c = 0; c<matrix.size(); c++){
                            if (l!=c ){
                                matrix.at(l).at(c) = 0;
                            }else{
                                matrix.at(l).at(c) = 1;
                            }
            }
        }
        return matrix;
}

/**
 * get the values of the arguments when the node is an op node 
 */
int print_arguments_string( isl_ast_op_type prev_op, isl_ast_expr *expr, std::vector<std::vector<int>> isl_ast_map)
{
    int i, n, p = 0;

    n = isl_ast_expr_get_op_n_arg(expr);

    if (n < 0) return 0;
    if (n == 0) return 0;
    p = 0;
    // going through all arguments of the op_node
    for (i = 0; i < n; ++i) {
        isl_ast_expr *arg;
        arg = isl_ast_expr_get_op_arg(expr, i);
        // for unary operations 
        if(i == 0){
            p = get_value(arg,isl_ast_map);
            if (prev_op == isl_ast_op_minus){p = - get_value(arg,isl_ast_map); break;}
        }
        // for binary operations
        else{
            switch(prev_op){
                case isl_ast_op_add:{p = p + get_value(arg,isl_ast_map); break;}
                case isl_ast_op_sub:{p = p - get_value(arg,isl_ast_map); break;}
                case isl_ast_op_mul:{p = p * get_value(arg,isl_ast_map); break;}
                case isl_ast_op_div:{if(get_value(arg,isl_ast_map)!= 0) p = p / get_value(arg,isl_ast_map); break;}
                case isl_ast_op_max:{p = std::max(p,get_value(arg,isl_ast_map)); break;}
                case isl_ast_op_min:{p = std::min(p,get_value(arg,isl_ast_map)); break;}
                case isl_ast_op_minus:{p = -get_value(arg,isl_ast_map); break;}
                default: p = get_value(arg,isl_ast_map); break;;
            }
        }
        isl_ast_expr_free(arg);
    }
    return p;
}
/**
 get an estimation of value of a variable in the expression of a bound 
*/
int get_id_value(std::string id, std::vector<std::vector<int>>isl_ast_map)
{
    // to estimate bounds that have variables in it, return an estimation of these variables (the mean of the values of the bounds for example)
    // in the case of only conestant bound, this function is not used
    return 0;
}
/**
 * get the numerical value of the expression 
 */
int get_value(isl_ast_expr *expr,std::vector<std::vector<int>> isl_ast_map){

    enum isl_ast_expr_type type;
    enum isl_ast_op_type op;
    isl_id *id;
    isl_val *v;
    std::string p;
    int val = 0;

    if (!expr){return -1;}
    else{
        type = isl_ast_expr_get_type(expr);
        switch (type) {
            case isl_ast_expr_error: return 0; break;
            case isl_ast_expr_op:
                op = isl_ast_expr_get_op_type(expr);
                if (op == isl_ast_op_error) return 0;
                val=val+print_arguments_string(op,expr,isl_ast_map);
                break;
            case isl_ast_expr_id:
                id = isl_ast_expr_get_id(expr);
                p = isl_id_get_name(id);
                val = get_id_value(p,isl_ast_map);
                break;
            case isl_ast_expr_int:
                v = isl_ast_expr_get_val(expr);val=1;
                val= isl_val_get_num_si(v);
                break;
            default: return 0;
            }
    return val;
    }
}

/**
 * get the bound from an isl expression as a string      
 */
std::string get_expr_isl_string( isl_ast_expr *expr,std::vector<std::vector<int>> isl_ast_map,bool is_bound)
{
    enum isl_ast_expr_type type;
    enum isl_ast_op_type op;
    isl_id *id;
    isl_val *v;
    std::string p;

    if (!expr){return "!Expression";}
    else{
        type = isl_ast_expr_get_type(expr);
        switch (type) {
            case isl_ast_expr_error: return "$Error in the expression"; break;
            case isl_ast_expr_op:
                op = isl_ast_expr_get_op_type(expr);
                if (op == isl_ast_op_error) return "$Error in the operation type";
                p = std::to_string( print_arguments_string( op, expr, isl_ast_map));
                break;
            case isl_ast_expr_id:
                if(!is_bound){
                    id = isl_ast_expr_get_id(expr);
                    p = isl_id_get_name(id);
                }
                else{
                    id = isl_ast_expr_get_id(expr);
                    p = isl_id_get_name(id);
                    p=std::to_string( get_id_value( isl_id_get_name( id ), isl_ast_map ));
                }
                break;
            case isl_ast_expr_int:
                v = isl_ast_expr_get_val(expr);
                p = std::to_string(isl_val_get_num_si(v));
                break;
            default: return "%";
        }
    return p;
    }
}
 /**
    get the loop bounds from the isl ast (uses code gen)
  */
 std::vector<std::vector<int>> get_ast_isl_bound_matrice(syntax_tree& ast){

    std::vector<std::vector<int>> isl_ast_bound_mat;
    std::vector<int> p1;
    isl_ast_expr * init_expr;
    isl_ast_expr * cond_expr;
    
    int stop = 0;
    // get the starting node of the new isl ast
    ast.fct->gen_isl_ast();
    isl_ast_node *ast_i = ast.fct->get_isl_ast();

    while(stop != 1)
    {
        if(isl_ast_node_get_type(ast_i) == isl_ast_node_for)
        {
            init_expr = isl_ast_node_for_get_init(ast_i); //Lower bound
            cond_expr = isl_ast_node_for_get_cond(ast_i); //Upper bound
            
            p1.push_back(std::stoi(get_expr_isl_string(init_expr,isl_ast_bound_mat,true)));
            p1.push_back(std::stoi(get_expr_isl_string(cond_expr,isl_ast_bound_mat,true)));
            isl_ast_bound_mat.push_back(p1);

            p1.clear();
            ast_i = isl_ast_node_for_get_body(ast_i);
        }
        else{stop = 1;}
    }   
    return isl_ast_bound_mat;
}
// list of matrices to explore at each level of the exploration tree
//std::vector <std::vector < std::vector<int> >> matrices;
// list of hashes of matrices we explored before to avoid repeating schedules 
std::vector<std::size_t> hashes;

void beam_search::search_save_matrix(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{

    //     
    std::default_random_engine rand_generator;

    
    std::vector<syntax_tree*> children;
    // list of ASTs to be explored for next level 
    std::vector<syntax_tree*> to_be_explored;
    

    int nb_explored_optims = ast.nb_explored_optims;
    
    // generate n matrices which is the max number of matrices to be explored
    optimization_type optim_type = optimization_type::MATRIX;

    children = scheds_gen->generate_schedules(ast, optim_type);
    std::hash<std::string> hasher;
    // hash the parent 
    std::size_t parent_hash=hasher(ast.get_schedule_str());
    // generate the matrices to be explored at this level
    std::vector <std::vector < std::vector<int> >> matrices = scheds_gen->get_matrices(ast, ast.get_program_depth());
    
    // if this is the roor of the exploration tree 
    if (ast.search_depth==0){

        optimization_info optim_info;
        optim_info.type = optimization_type::MATRIX;
        optim_info.comps = ast.computations_list;
        // for the original schedule, the transformation matrix is the identity
        optim_info.matrix = get_identity(ast.get_program_depth());
        ast.new_optims.push_back(optim_info);
        // the root to the hashes to avoid repeating the identity matrix
        hashes.push_back(hasher(ast.get_schedule_str()));
    }
    
    children.resize(std::min((int)matrices.size(), (int)children.size()));
    
    
    // stop if no more optimizations can be applied
    if (children.size() == 0) return ;
    


    
    auto iterator = children.begin();
    

    // Getting the initial loop bounds 
    std::vector<std::vector<int>> bounds_mat;
    bounds_mat = get_ast_isl_bound_matrice(ast);
   
    
    std::vector<std::vector<std::vector<int>>> repeated;
     
    
    
    // number of matrices explored so far at this level. used to go through the matrices global variable
    int nb_matrices =0;

    syntax_tree *child = *iterator;


    // evaluate children and sort them from smallest to highest evaluation
    // evaluate while removing illegal versions
    while (iterator != children.end())
    {
         
        child = *iterator;
        
        
        
        child->nb_explored_optims = nb_explored_optims;
        
        
        
        //add the matrix to optim.info
        child->new_optims.back().matrix = matrices.at(nb_matrices);
        nb_matrices++;
        
        // get the bounds of the original program
        child->bounds_matrix = bounds_mat;
            
        // multipy the bounds by the matrix to get the transformed bounds estimation    
        if(child->transformed_bounds_matrix.size()==0){
            child->transformed_bounds_matrix = multiply(child->new_optims.back().matrix,child->bounds_matrix);
        }else{
            child->transformed_bounds_matrix = multiply( child->new_optims.back().matrix,child->transformed_bounds_matrix);
        }
        

        child->transform_ast();

        if (!child->ast_is_legal()) {
            if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                // print deleted Ast
                child->print_previous_optims();
                std::cout << "\n-----------" << std::endl;
                std::cout<<ast.get_schedule_str()<<std::endl;
                child->print_new_optims();
                
                child->print_ast();
                child->print_isl_states();
                std::cout << "\n<illegal>\n";
            }
            delete child;
            iterator = children.erase(iterator);
        }
        else {
            // if the transformation is legal, add the new matrix to the transformation list
            if(child->transformation_matrix.size()==0){
                child->transformation_matrix = child->new_optims.back().matrix;
            }else{
                child->transformation_matrix = multiply( child->new_optims.back().matrix,child->transformation_matrix);
            }
            
        
            
            
            // hash the legal matrix 
            std::size_t hash=hasher(child->get_schedule_str());
            

            bool repeated = false;
            // check if we explored this matrix before  
            for(std::size_t hashe:hashes){
                
                if(hashe==hash){
                    delete child;
                    iterator = children.erase(iterator);
                    repeated = true;
                    break;
                    
                }
            }
        
            if(repeated) continue;

            
            // if the matrix is legal and not repeated we add its hash to the list of seen hashes and we start the evaluation 
            hashes.push_back(hash);
               
            // print and evaluate Ast
            if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                child->print_previous_optims();
                std::cout << "\n-----------" << std::endl;
                std::cout<<ast.get_schedule_str()<<std::endl;
                child->print_new_optims();
                //std::cout<<child->get_schedule_str()<<std::endl;
                child->print_ast();
                child->print_isl_states();
                std::cout << "\n<legal>\n";
                child->print_computations_accesses();
            }
            int fd[2];
            
            // create pipe descriptors
	        pipe(fd);
            timeout=0;
            child_done=0;
            cont = false;
            std::vector<float> measurements;
            
            
            // create a child process to execute the code and get measurements
            // this is added so that we can limit the code generation time for large programs
            pid_t pid;
            pid = fork();

            if (pid == -1) {
                perror("fork failed");
                exit(1);
            } else if (pid == 0) {
                measurements = exec_eval->get_measurements_matrix(*child, false, schedule_timeout);
                int size =measurements.size();
                float ar[measurements.size()];

                // put the measurements in an array to be sent to the parent process
                
                for(int i=0;i<measurements.size();i++) ar[i]=measurements.at(i);
                
                close(fd[0]);
                // send number of measurements to parent process
                write(fd[1], &size, sizeof(size));
                // send measurements to parent process
                write(fd[1], &ar, sizeof(ar));
                // close the pipe
                close(fd[1]);
                // end child process
                _exit(1);
            }

            // set up the signal handlers after forking so the child doesn't inherit them
            // an alarm in the case of the timelimit is reached and the child process was interrupted
            signal(SIGALRM, alarm_handler);
            // an alarm in the case of the child process ending ie. both data generation and evaluation are done
            signal(SIGCHLD, child_handler);
            // an alarm in the case of the data generation ending before the timelimit. We still need to wait for the evaluation to be done. 
            signal(SIGUSR1,sig_usr);
            // install an alarm to be fired after TIME_LIMIT
            
            // set the alarm
            alarm(TIME_LIMIT);
            
            pause();

            if (timeout) {
                
                // if the timeout has been reached    
                    
                int result = waitpid(pid, NULL, WNOHANG);
                if (result == 0) {
                    // child still running, so kill it
                    
                    
                    // Remove all the optimizations
                    exec_eval->fct->reset_schedules();
                    measurements.clear();
                    // if the timeout has been reached, put infinity as a measurement

                    measurements.push_back(std::numeric_limits<float>::infinity());
                    // cancel any previously set alarm 
                    alarm(0); 
                    // kill child process
                    kill(pid, 9);
                    
                
                    waitpid(-1,NULL,0);
                } else {
                    // if by the time we detect that the alarm has been raised, the evaluation has been completed
                    // we recieve the measurements from the child
                    int size = 0;
                    close(fd[1]);
                    read(fd[0], &size, sizeof(int));
                    float ar[size];
                    read(fd[0], &ar, size*sizeof(float));
                    for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                    close(fd[0]);
                    
                }
                
                
            }else if (child_done) {
                // the execution of the child is done, both code generation and evaluation
                // we recieve the measurements from the child
                int size =0;
                close(fd[1]);
                read(fd[0], &size, sizeof(int));
                float ar[size];
                read(fd[0], &ar, size*sizeof(float));
                for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                close(fd[0]);
                
                waitpid(-1,NULL,0);
            }else if(cont){
                // execution is not done but the code generation is done. This happens for large programs that take a long time to execute
                // cancel the timeout alarm  
                alarm(0);
                int size=0;
                // we wait for the evalution of the generated code
                while(!child_done){}
                // after evealuation is done, we recieve the measurements from the child
                close(fd[1]);
                read(fd[0], &size, sizeof(int));
                float ar[size];
                read(fd[0], &ar, size*sizeof(float));
                for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                close(fd[0]);
                waitpid(-1,NULL,0);
            }
                    
            child->evaluation = min_eval(measurements);

            if(hash != parent_hash) child->nb_explored_matrices = child->nb_explored_matrices +1; 
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
            to_be_explored.push_back(child);
            ++iterator;  
            
        }
    }
    syntax_tree *ast_copy = ast.copy_ast();
    ast_copy->nb_explored_optims = nb_explored_optims;
                    
    to_be_explored.push_back(ast_copy);
    
                    
    parent_trace->add_child_path(ast_copy, parent_trace->get_candidate_id());
    


    
    // Sort children from smallest evaluation to largest
    std::sort(to_be_explored.begin(), to_be_explored.end(), [](syntax_tree *a, syntax_tree *b) {
       return a->evaluation < b->evaluation;
    });

    // shuffle the children so that they are selected a random
    //std::shuffle(std::begin(to_be_explored), std::end(to_be_explored), rand_generator);
    
    // keep the top 'beam_size' children and delete the rest
    for (int i = beam_size; i < to_be_explored.size(); ++i)
       delete to_be_explored[i];
    
    
    

    to_be_explored.resize(std::min(beam_size, (int)to_be_explored.size()));
   
    
    for (syntax_tree *child : to_be_explored)
    {
        // increment the search depth for the recursive call 
        child->search_depth = ast.search_depth + 1;
        // if we are under the maximum depth of matrices to explore then call search_save_matrix recursivly
        if (child->search_depth<MAX_MAT_DEPTH && child->search_depth<=child->nb_explored_matrices ){
            search_save_matrix(*child, schedules_annotations, parent_trace->child_mappings[child], schedule_timeout);
        }else{
            // if we surpassed the MAX_MAT_DEPTH amount of matrices to explore OR we detected the parent of this level through
            // the child->search_depth<=child->nb_explored_matrices condition which means that the search level is greater than the number of applied matrices
            search_save(*child, schedules_annotations, parent_trace->child_mappings[child], schedule_timeout);  
        }
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