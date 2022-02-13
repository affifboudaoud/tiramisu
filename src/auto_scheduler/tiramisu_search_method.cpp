#include <sys/wait.h>
#include <tiramisu/auto_scheduler/search_method.h>
#include <random>
#include <functional>
#include <exception>

#include <stdexcept>
#define TIME_LIMIT 1000
struct TimeLimitException : public std::exception
    {
        const char * what () const throw ()
        {
            return "passed timelimit while measuring the execution time";
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
bool changes_mes = false;
void beam_search::search_save(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::default_random_engine rand_generator;
    
    //if (ast.nb_explored_optims % NB_OPTIMIZATIONS == 0)
    //    ast.clear_new_optimizations();

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
            
            
            pid_t pid, ppid;
            ppid = getpid();
            pid = fork();

            if (pid == -1) {
                perror("fork failed");
                exit(1);
            } else if (pid == 0) {
                measurements = exec_eval->get_measurements_matrix(*child, false, schedule_timeout);
                int size =measurements.size();
                float ar[measurements.size()];
                for(int i=0;i<measurements.size();i++) ar[i]=measurements.at(i);
                close(fd[0]);
                write(fd[1], &size, sizeof(size));
                
                write(fd[1], &ar, sizeof(ar));
                
                close(fd[1]);
                _exit(1);
            }

            // set up the signal handlers after forking so the child doesn't inherit them

            signal(SIGALRM, alarm_handler);
            signal(SIGCHLD, child_handler);
            signal(SIGUSR1,sig_usr);
              // install an alarm to be fired after TIME_LIMIT
            
            alarm(TIME_LIMIT);
            
            pause();

            if (timeout) {
                
                    
                int result = waitpid(pid, NULL, WNOHANG);
                if (result == 0) {
                    // child still running, so kill it
                    
                    
                    // Remove all the optimizations
                    exec_eval->fct->reset_schedules();
                    measurements.clear();
                    measurements.push_back(std::numeric_limits<float>::infinity());
                    // cancel any previously set alarm 
                    alarm(0); 
                    kill(pid, 9);
                    //kill(pid, SIGKILL);
                    
                
                    waitpid(-1,NULL,0);
                } else {
                    
                    int size = 0;
                    close(fd[1]);
                    read(fd[0], &size, sizeof(int));
                    float ar[size];
                    read(fd[0], &ar, size*sizeof(float));
                    for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                    close(fd[0]);
                    
                }
                
                
            }else if (child_done) {
                
                int size =0;
                close(fd[1]);
                read(fd[0], &size, sizeof(int));
                float ar[size];
                read(fd[0], &ar, size*sizeof(float));
                for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                close(fd[0]);
                
                waitpid(-1,NULL,0);
            }else if(cont){ 
                alarm(0);
                int size=0;
                
                while(!child_done){}
                
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
int determinant( std::vector <  std::vector<int> >  matrix, int n) {
   int det = 0;
   int o;
   if(n==1) return matrix.at(0).at(0);
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
bool check_if_repeated( std::vector < std::vector<int> >  matrix,std::vector < std::vector < std::vector<int> > > matrices)
{
    //if there are no matrices to compare to then we return false
    if(matrices.size()==0) return false;
    
    int depth = matrix.size();
    int i=0;
    while(i<matrices.size()){
        //for each matrix that we already explored  
        bool diffrent = false;
        for (int s=0;s<depth;s++){
                for (int d=0;d<depth;d++){
                            // set diffrent to true if we find one element that is not the same
                            if (matrix.at(s).at(d)!=matrices.at(i).at(s).at(d)) diffrent =true ;
                    }
        }
        //if we found the same matrix return true
        if (!diffrent) return true;
    i++;
    }

     
    return false;
}
std::vector < std::vector < std::vector<int> > > get_random_matrcies(int nb_out_matrcies, int depth)
{
    std::vector <std::vector <  std::vector<int> >>  result(nb_out_matrcies);
    int nb_valid_matrices = 0;
    int max_depth = 6;
    if (depth>max_depth) std::cout << "WARNING: the depth of this program is too big. Matrix generation will take a long time \n"<< std::endl;
    srand((unsigned) time(0));
    while(nb_valid_matrices<nb_out_matrcies)
    {
        std::vector <  std::vector<int> >  random(depth);
        bool valid = false;
        while (!valid)
        {   
            //std::cout << "generating";
            int l, c;
            std::vector <  std::vector<int> >  randomL(depth);
            for(l = 0; l<depth; l++){
                randomL.at(l)= std::vector<int>(depth);
                for(c = 0; c<depth; c++){
                                if (l>c){
                                    randomL.at(l).at(c) = (rand() %10) - 5;
                                }else{
                                    randomL.at(l).at(c) = 0;
                                }
                }
            }
             
            std::vector <  std::vector<int> >  randomU(depth);
            for(l = 0; l<depth; l++){
                randomU.at(l)= std::vector<int>(depth);
                for(c = 0; c<depth; c++){
                            if (l<c){
                                randomU.at(l).at(c) = (rand() % 10) - 5;
                            }else{
                                randomU.at(l).at(c) = 0;
                            }
                }
            }
            for(l = 0; l<depth; l++){
                randomL.at(l).at(l) =1;
                randomU.at(l).at(l) =1;
                int sum=0,j=0;
                if(l<depth-1){
                    sum=0;
                    for(j=0;j<l;j++){
                        sum+= randomL.at(l).at(j) * randomU.at(j).at(depth-1);
                    }
                    randomU.at(l).at(depth-1) = (1-sum)/randomL.at(l).at(l);
                }else{
                    sum=0;
                    for(j=1;j<l+1;j++){
                        sum+= randomL.at(l).at(j) * randomU.at(j).at(depth-1);
                    }
                    randomL.at(depth-1).at(0) = (1-sum)/randomU.at(0).at(depth-1);
                }
            }
            int k;
            for(l = 0; l < depth; ++l){
                random.at(l)= std::vector<int>(depth);
                for(c = 0; c < depth; ++c)
                    {
                        for(k = 0; k < depth; ++k)
                        {
                            random.at(l).at(c)+= randomL.at(l).at(k) * randomU.at(k).at(c);
                        }
                    }
            }
            // Reduce elements into [-7, 7] values
            int mx = 7;
            int m = -1000;
            int x,y;
            for(int i = 0; i < depth; i++){
                        for(int j = 0; j< depth; j++){
                            if(m < std::abs(random.at(i).at(j)))
                            {
                                m = random.at(i).at(j);
                                x = i;
                                y = j;
                            }
                        }
            }
            int mm=-8;
            int useless = 0;
            int steps = 0;
            bool bad_case;
            int f = -1;
            // Check determinant equals 1
            //std::cout<<"Before \n"<< determinant(random, depth)<<std::endl;
            while(m>=mx && useless<= depth*depth){
                    steps+=1;
                    bad_case=0;
                    for(int i = 0; i < depth; i++){
                        if (i==x) continue;
                        if(random.at(i).at(y)!=0)
                            {
                                f = i;
                                break;
                            }
                        if(i==depth-1) bad_case=1;
                    }
                    //std::cout<<"First \n"<<std::endl;
                    if(bad_case) break;
                    int s =1;
                    //std::cout<<f<<" Second \n"<<y<<std::endl;
                    if (m*random.at(f).at(y)<0) s=s*-1;
                    
                    for(int j = 0; j < depth; j++){
                            
                            random.at(x).at(j) = random.at(x).at(j) - s * random.at(f).at(j);
                    }
                    //std::cout<<"Second \n"<<std::endl;
                    mm=-10000;
                    for(int i = 0; i < depth; i++){
                        for(int j = 0; j< depth; j++){
                            if(mm < std::abs(random.at(i).at(j)))
                            {
                                mm = random.at(i).at(j);
                                x = i;
                                y = j;
                            }
                        }
                    }
                    //std::cout<<"Third \n"<<std::endl;
                    if(m>=mm) useless++;
                    m=mm;
            }
            // Check determinant equals 1
            int det = determinant(random, depth);
            bool det_bool = det==1 && !bad_case && (useless <= depth*depth) ;
            //std::cout<< "After \n"<<det_bool<<std::endl;
            // Check upper right determinants equal 1
            bool all_1 = true;
            if (det_bool){
                //std::cout << depth;
                int d=0,s=0;
                
                for (k=depth-1;k>0;k--){
                        //std::cout<<" sub matrix at depth: " <<k <<"\n";
                        std::vector <  std::vector<int> >  submatrixd(depth-k);
                        
                        for (s=0;s<depth-k;s++){
                                    submatrixd.at(s) = std::vector<int> (depth-k);
                                    for (d=0;d<depth-k;d++){
                                            
                                            submatrixd.at(s).at(d) = random.at(s).at(k+d); 
                                    } 
                        }  
                        
                        if(determinant(submatrixd, depth-k)!=1){ 
                            all_1 = false;
                            /*std::cout<< "failed at size: "<< depth-k<<"with determinant being:"<< determinant(submatrixd, depth-k) <<"\n";
                            for (s=0;s<depth-k;s++){
                                for (d=0;d<depth-k;d++){
                                            std::cout<< submatrixd.at(s).at(d) <<"\n";
                                    }
                            }*/
                        }
                        if(!all_1) break;
                }
            } 
            valid = det_bool && all_1 ;
        }
    //std::cout<< "got one done \n"<<std::endl;
    /*std::cout<< "starts \n";
    for(int i = 0; i < depth; i++){
                        for(int j = 0; j< depth; j++){
                                std::cout<<random.at(i).at(j)<<"\n"<<std::endl;
                             
                        }
            }
    std::cout<< "end \n";*/
    result.at(nb_valid_matrices) = random;
    nb_valid_matrices++;
    }
    return result;
}
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
std::vector<std::vector<int>>  multiply_plus(const std::vector<std::vector<int>> & m1, const std::vector<std::vector<int>> & m2,const std::vector<std::vector<int>>  last_col)
        {
        std::vector<std::vector<int>> result(m1.size(), std::vector<int>(m2.at(0).size()+1));

            for(std::size_t row = 0; row < result.size(); ++row) {
                for(std::size_t col = 0; col < result.at(0).size()-1; ++col) {
                    for(std::size_t inner = 0; inner < m2.size(); ++inner) {
                        result.at(row).at(col) += m1.at(row).at(inner) * m2.at(inner).at(col);
                    }
                }
                result.at(row).at(result.at(0).size()-1)= last_col[row][last_col.at(0).size()-1];
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
Generate one random matrix that verifies the conditions of: 1- determinant is one 2- all of the upper left determinants are 1
*/
  std::vector < std::vector<int> >  get_neighbor_matrix_random_element(std::vector < std::vector<int> >matrix ,int depth)
{
    //std::cout<<"stratred random"<<std::endl;
    std::vector < std::vector<int> > result(depth);
    for(int l = 0; l<depth; l++){
            result.at(l) = std::vector<int>(depth);
            for(int c = 0; c<depth; c++){
                            result.at(l).at(c) = matrix.at(l).at(c);
            }
        }
       
    int mx = 1;
    int m = 0;
    int x =-1,y=-1;
    
    
    int mm=-8;
    int useless = 0;
    int steps = 0;
    bool bad_case=0;
    int f = -1;
    bool found_elgibile = false;
    while(useless<= depth*depth){
            x = rand()%depth;
            y = rand()%depth;
            m = result.at(x).at(y);
            bad_case=0;
            for(int i = 0; i < depth; i++){
                if (i==x) continue;
                if(result.at(i).at(y)!=0)
                {
                        f = i;
                        break;
                }
                if(i==depth-1) bad_case=1;
            }
            
            if(bad_case || f==-1) {
                useless++;
                continue;
            }
            
            int s =1;
            if (m*result.at(f).at(y)<0) s=s*-1;
            
            for(int j = 0; j < depth; j++){
                   result.at(x).at(j) = result.at(x).at(j) - s * result.at(f).at(j);
            }
            
            mm = matrix.at(x).at(y);
            
            if(m!=mm){return result;}else{continue;} 
            }
    if (useless>= depth*depth) std::cout<<"useless"<<std::endl;
    x = rand()%depth;
    y = rand()%depth;
    result.at(x).at(y)=result.at(x).at(y)-(rand()%2 -1);
    return result;
    
}
/*
Generate one random matrix that verifies the conditions of: 1- determinant is one 2- all of the upper left determinants are 1
*/
  std::vector < std::vector<int> >  get_neighbor_matrix(std::vector < std::vector<int> >matrix ,int depth)
{
    std::vector < std::vector<int> > result(depth);
    for(int l = 0; l<depth; l++){
            result.at(l) = std::vector<int>(depth);
            for(int c = 0; c<depth; c++){
                            result.at(l).at(c) = matrix.at(l).at(c);
            }
        }
       
    int mx = 1;
    int m = 0;
    int x =-1,y=-1;
    for(int i = 0; i < depth; i++){
                for(int j = 0; j< depth; j++){
                        std::cout<<result.at(i).at(j);
                }
                std::cout<<std::endl;
    }
    for(int i = 0; i < depth; i++){
                for(int j = 0; j< depth; j++){
                    if(std::abs(m) < std::abs(result.at(i).at(j)))
                    {
                        m = result.at(i).at(j);
                        //std::cout<<"biggest elemnt in the matrix iside: "<<m<<std::endl;
                        x = i;
                        y = j;
                    }
                }
    }
    //std::cout << "X: "<<x<<"y: "<<y<<"m:"<<m<<std::endl;
    //std::cout<<"biggest elemnt in the matrix is: "<<m<<std::endl;
    int mm=-8;
    int useless = 0;
    int steps = 0;
    bool bad_case=0;
    int f = -1;
    
    while(useless<= depth*depth){
            steps+=1;
            bad_case=0;
            for(int i = 0; i < depth; i++){
                if (i==x) continue;
                if(result.at(i).at(y)!=0)
                {
                        f = i;
                        break;
                }
                if(i==depth-1) bad_case=1;
            }
            
            if(bad_case) break;
            int s =1;
            //std::cout<<"x: "<<x <<std::endl;
            //std::cout<<"f: "<<f <<std::endl;
            //std::cout<<"y: "<<y <<std::endl;
            if (m*result.at(f).at(y)<0) s=s*-1;
            
            for(int j = 0; j < depth; j++){
                    
                   result.at(x).at(j) = result.at(x).at(j) - s * result.at(f).at(j);
            }
            
            mm =0;
            int new_x=-1, new_y=-1;
            for(int i = 0; i < depth; i++){
                for(int j = 0; j< depth; j++){
                    if(std::abs(mm) < std::abs(result.at(i).at(j)))
                    {
                        mm = result.at(i).at(j);
                        new_x = i;
                        new_y = j;
                    }
                }
            }
            //std::cout << "new X: "<<new_x<<"new y: "<<new_y<<"mm:"<<mm<<std::endl;
            for(int i = 0; i < depth; i++){
                for(int j = 0; j< depth; j++){
                        std::cout<<result.at(i).at(j);
                }
                std::cout<<std::endl;
    }
            if(!(x== new_x && y==new_y && m==mm)){return result;}else{return get_neighbor_matrix_random_element(matrix, depth);} 
            m=mm;
            x = new_x;
            y = new_y;
            }
    if(bad_case) return get_neighbor_matrix_random_element(matrix, depth);
    return result;
    
}
/*
Generate one random matrix that verifies the conditions of: 1- determinant is one 2- all of the upper left determinants are 1
*/
  std::vector < std::vector<int> >  get_neighbor_matrix_random_element_diag_lower(std::vector < std::vector<int> >matrix ,int depth)
{
    //std::cout<<"stratred random"<<std::endl;
    std::vector < std::vector<int> > result(depth);
    for(int l = 0; l<depth; l++){
            result.at(l) = std::vector<int>(depth);
            for(int c = 0; c<depth; c++){
                            result.at(l).at(c) = matrix.at(l).at(c);
            }
        }
       
    int x = rand()%(depth-1)+1;
    int y = rand()%x;
    
    int m=1;
    if(rand()%2!=0) m=-1;
    result.at(x).at(y)= result.at(x).at(y) - m; 
    return result;
    
}
/*
Generate one random matrix that verifies the conditions of: 1- determinant is one 2- all of the upper left determinants are 1
*/
  std::vector < std::vector<int> >  get_neighbor_matrix_random_element_diag_upper(std::vector < std::vector<int> >matrix ,int depth)
{
    //std::cout<<"stratred random"<<std::endl;
    std::vector < std::vector<int> > result(depth);
    for(int l = 0; l<depth; l++){
            result.at(l) = std::vector<int>(depth);
            for(int c = 0; c<depth; c++){
                            result.at(l).at(c) = matrix.at(l).at(c);
            }
        }
       
    int y = rand()%(depth-1)+1;
    int x = rand()%x;
    int m=1;
    if(rand()%2!=0) m=-1;
    result.at(x).at(y)= result.at(x).at(y) - m; 
    return result;
    
}
std::vector < std::vector<int> >  get_random_matrix(int depth)
{
    int max_depth = 6;
    if (depth>max_depth) std::cout << "WARNING: the depth of this program is too big. Matrix generation will take a long time \n"<< std::endl;
    std::vector <  std::vector<int> >  random(depth);
    bool valid = false;
    while (!valid)
    {   

        //generate a lower traiangular matrix
        int l, c;
        int choice = rand() %100;
        std::vector <  std::vector<int> >  randomL(depth);
        for(l = 0; l<depth; l++){
            randomL.at(l)= std::vector<int>(depth);
            for(c = 0; c<depth; c++){
                randomL.at(l).at(c) = (rand() %20) - 10;          
            }
        }
        if(determinant(randomL, depth)==1) continue;
        return randomL;
        /*
        randomU.at(0)= std::vector<int>(depth);randomU.at(0).at(0)=1;randomU.at(0).at(1)=0;randomU.at(0).at(2)=0;
        randomU.at(1)= std::vector<int>(depth);randomU.at(1).at(0)=-6;randomU.at(1).at(1)=1;randomU.at(1).at(2)=0;
        randomU.at(2)= std::vector<int>(depth);randomU.at(2).at(0)=5;randomU.at(2).at(1)=-5;randomU.at(2).at(2)=1;
        return randomU;
        */
    }
    return random;
    
}
/*
Generate one random matrix that verifies the conditions of: 1- determinant is one 2- all of the upper left determinants are 1
*/
  std::vector < std::vector<int> > get_random_matrix_det_1(int depth)
{
    int max_depth = 6;
    if (depth>max_depth) std::cout << "WARNING: the depth of this program is too big. Matrix generation will take a long time \n"<< std::endl;
    std::vector <  std::vector<int> >  random(depth);
    bool valid = false;
    while (!valid)
    {   

        //generate a lower traiangular matrix
        int l, c;
        int choice = rand() %100;
        std::vector <  std::vector<int> >  randomL(depth);
        for(l = 0; l<depth; l++){
            randomL.at(l)= std::vector<int>(depth);
            for(c = 0; c<depth; c++){
                            if (l>c){
                                randomL.at(l).at(c) = (rand() %14) - 7;
                            }else{
                                if(l<c){
                                    randomL.at(l).at(c) = 0;
                                }else{
                                    randomL.at(l).at(c)=1;
                                }
                            }
            }
        }

        if(choice>3 && choice< 5 && !is_identity(randomL))  return randomL;
        ////generate an upper traiangular matrix
        std::vector <  std::vector<int> >  randomU(depth);
        for(l = 0; l<depth; l++){
            randomU.at(l)= std::vector<int>(depth);
            for(c = 0; c<depth; c++){
                        if (l<c){
                            randomU.at(l).at(c) = (rand() % 14) - 7;
                        }else{
                            if(l>c){
                                    randomU.at(l).at(c) = 0;
                                }else{
                                    randomU.at(l).at(c)=1;
                                }
                        }
            }
        }
        if(choice<3 && !is_identity(randomU)) return randomU;
        randomU = multiply(randomL,randomU);
        if(choice>5 && !is_identity(randomU))return randomU;
        if(!is_identity(randomU)) continue;
        /*
        randomU.at(0)= std::vector<int>(depth);randomU.at(0).at(0)=1;randomU.at(0).at(1)=0;randomU.at(0).at(2)=0;
        randomU.at(1)= std::vector<int>(depth);randomU.at(1).at(0)=-6;randomU.at(1).at(1)=1;randomU.at(1).at(2)=0;
        randomU.at(2)= std::vector<int>(depth);randomU.at(2).at(0)=5;randomU.at(2).at(1)=-5;randomU.at(2).at(2)=1;
        return randomU;
        */
    }
    return random;
    
}


        // Gettig the values of the isl AST
    int print_arguments_string(isl_ast_op_type prev_op,isl_ast_expr *expr,std::vector<std::vector<int>> isl_ast_map )
    {
        int i, n, p = 0;

        n = isl_ast_expr_get_op_n_arg(expr);

        if (n < 0) return 0;
        if (n == 0) return 0;
        p = 0;
        for (i = 0; i < n; ++i) {
            isl_ast_expr *arg;
            //std::cout<<"---------------Arg";
            //std::cout<<i;
            //std::cout<<"\n";
            arg = isl_ast_expr_get_op_arg(expr, i);
            if(i==0){
                p = get_value(arg,isl_ast_map);
                if (prev_op == isl_ast_op_minus){p = -get_value(arg,isl_ast_map);break;}
            }
            else{
                switch(prev_op){
                    case isl_ast_op_add:{p = p+get_value(arg,isl_ast_map);break;}
                    case isl_ast_op_sub:{p = p-get_value(arg,isl_ast_map);break;}
                    case isl_ast_op_mul:{p = p*get_value(arg,isl_ast_map);break;}
                    case isl_ast_op_div:{if(get_value(arg,isl_ast_map)!= 0)p = p / get_value(arg,isl_ast_map);break;}
                    case isl_ast_op_max:{p = std::max(p,get_value(arg,isl_ast_map));break;}
                    case isl_ast_op_min:{p = std::min(p,get_value(arg,isl_ast_map));break;}
                    case isl_ast_op_minus:{p = -get_value(arg,isl_ast_map);break;}
                    default: p = get_value(arg,isl_ast_map);break;;
                }
            }
            isl_ast_expr_free(arg);
        }
        return p;
    }
    //get the Upper bound of an id
    int get_id_value(std::string id,std::vector<std::vector<int>>isl_ast_map)
    {
        std::map <int,  std::tuple<std::string , std::string,std::string> >::iterator it;

        /*for (it = isl_ast_map.begin(); it != isl_ast_map.end(); it++)
        {
            if(std::get<2>(it->second) == id){
                return (std::stoi(std::get<0>(it->second)) + std::stoi(std::get<1>(it->second))) / 2;
            }
        }*/
        return 0;
    }

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
                    //std::cout<<"Entreing OP : ";
                    //std::cout<<op;
                    //std::cout<<"\n";
                    break;
                case isl_ast_expr_id:
                    id = isl_ast_expr_get_id(expr);
                    p = isl_id_get_name(id);
                    val = get_id_value(p,isl_ast_map);
                    //std::cout<<"Entreing Id with";
                    //std::cout<<val;
                    //std::cout<<"\n";
                    break;
                case isl_ast_expr_int:
                    v = isl_ast_expr_get_val(expr);val=1;
                    val= isl_val_get_num_si(v);
                    //std::cout<<"Entreing Int with";
                    //std::cout<<val;
                    //std::cout<<"\n";
                    break;
                default: return 0;
                }
        return val;
        }
    }

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
                    p = std::to_string(print_arguments_string(op,expr,isl_ast_map));
                    //std::cout<<"Entreing OP with ";
                    //std::cout<<op;
                    //std::cout<<"\n";
                    break;
                case isl_ast_expr_id:
                    if(!is_bound){
                        id = isl_ast_expr_get_id(expr);
                        p = isl_id_get_name(id);
                    }
                    else{
                        id = isl_ast_expr_get_id(expr);
                        p = isl_id_get_name(id);
                        p=std::to_string(  get_id_value(isl_id_get_name(id),isl_ast_map));
                    }

                    //std::cout<<"Entreing Id with ";
                    //std::cout<<p;
                    //std::cout<<"\n";
                    break;
                case isl_ast_expr_int:
                    v = isl_ast_expr_get_val(expr);
                    p = std::to_string(isl_val_get_num_si(v));
                    //std::cout<<"Entreing Int with";
                    //std::cout<<p;
                    //std::cout<<"\n";
                    break;
                default: return "%";
            }
        return p;
        }
    }
    std::vector<int> format_bound(int bound, int id_rank, int size, bool is_lower, bool with_bounds){
          std::vector<int> output;
          
          for(int i=0;i< size ;i++){
              if(i==id_rank){
                if(is_lower){ output.push_back(-1);}
                else{output.push_back(1);}               
              }else{
                output.push_back(0);
              }
          }
          if(with_bounds){
            if(is_lower){ output.push_back(-bound);}
            else{ output.push_back(bound);}
        }
        return output;
    }
    
 std::vector<std::vector<int>> get_ast_isl_bound_matrice(syntax_tree& ast){

        std::vector<std::vector<int>> isl_ast_mat;
        std::vector<int>p1;
        isl_ast_expr * init_expr;
        isl_ast_expr * cond_expr;
        isl_ast_expr * iter_expr;
        int stop = 0;

        ast.fct->gen_isl_ast();

        isl_ast_node *ast_i = ast.fct->get_isl_ast();
        while(stop!=1)
        {

            if(isl_ast_node_get_type(ast_i) == isl_ast_node_for)
            {
                init_expr=isl_ast_node_for_get_init(ast_i); //Lower bound
                cond_expr=isl_ast_node_for_get_cond(ast_i); //Upper bound
                iter_expr=isl_ast_node_for_get_iterator(ast_i); //Get the ID name

                p1.push_back(std::stoi(get_expr_isl_string(init_expr,isl_ast_mat,true)));
                p1.push_back(std::stoi(get_expr_isl_string(cond_expr,isl_ast_mat,true)));
                isl_ast_mat.push_back(p1);
                p1.clear();
                ast_i= isl_ast_node_for_get_body(ast_i);
            }
            else{stop=1;}
        }
        
     
        return isl_ast_mat;
}
std::pair< std::vector<std::vector<int>>,std::vector<std::vector<int>>> get_ast_isl_constraint_matrice( std::vector<std::vector<int>> isl_ast_mat){

        std::vector< std::vector<int>> constraint_mat;
        std::vector< std::vector<int>> constraint_mat_with_bounds;
       
        for (int i = 0; i < isl_ast_mat.size(); i++) {
        for (int j = 0; j < isl_ast_mat[i].size(); j++)
            {
               if (j==0){  
                    constraint_mat.push_back(format_bound(isl_ast_mat[i][j],i,isl_ast_mat.size(),true,false));  
                    constraint_mat_with_bounds.push_back(format_bound(isl_ast_mat[i][j],i,isl_ast_mat.size(),true,true));  
                } else{
                    constraint_mat.push_back(format_bound(isl_ast_mat[i][j],i,isl_ast_mat.size(),false,false)); 
                    constraint_mat_with_bounds.push_back(format_bound(isl_ast_mat[i][j],i,isl_ast_mat.size(),false,true)); 
                }       
            }
        }
        return std::pair< std::vector<std::vector<int>>,std::vector<std::vector<int>>> (constraint_mat,constraint_mat_with_bounds);
}

void beam_search::search_save_matrix(syntax_tree& ast, std::vector<std::string> *schedules_annotations, candidate_trace *parent_trace, float schedule_timeout)
{
    std::default_random_engine rand_generator;


    std::vector<syntax_tree*> children;
    std::vector<syntax_tree*> to_be_explored;

    // Look for an optimization that can be applied
    int nb_optims_tried = 0;
    int nb_explored_optims = ast.nb_explored_optims;
    auto start = std::chrono::system_clock::now();
    //Generate n matrice asts to be explored
    //To change the number of matrices being explored go to: generate_schedules then the MATRIX case and change the length of the loop
    optimization_type optim_type = optimization_type::MATRIX;

    children = scheds_gen->generate_schedules(ast, optim_type);
    
    
    // Stop if no more optimizations can be applied
    //Add the current AST to the list of children
    if (children.size() == 0)
        return ;
    
    // Evaluate children and sort them from smallest to highest evaluation
    // evaluate while removing illegal versions
    auto iterator = children.begin();
    std::vector < std::vector < std::vector<int> > > matrices;

    //std::map <std::string,std::string>* corr_map;
    std::vector<std::vector<int>> bounds_mat;
    std::pair<std::vector<std::vector<int>>,std::vector<std::vector<int>> > constraint_mats;
    bounds_mat = get_ast_isl_bound_matrice(ast);
    constraint_mats = get_ast_isl_constraint_matrice(bounds_mat);
    
     
    // Add the corr_map to the ast structue
    //corr_map = get_corr_map_from_isl(ast);
    //Hash the program string to get a unique seed for each program 
    std::hash<std::string> hasher;
    auto hashed = hasher(evaluate_by_learning_model::get_program_json(ast));
    srand(hashed);
    int nb_matrices =0;
    int nb_steps = 0;
    bool illegal = false;
    bool first_time_illegal = true;
    syntax_tree *child = *iterator;
    
    while (iterator != children.end())
    {

        //If we tried to find a new matrix too many times, we give up and explore the ones we found so far 
        if (nb_steps++>MAX_NB_STEPS){
            break;
        } 
        if (!illegal)  child = *iterator;

        // Add the corr_map to the ast structue
        //child->corr_map = corr_map;

        child->nb_explored_optims = nb_explored_optims;
        
        int shape = child->get_program_depth();
        //save an AST in case the matrix is illegal
        syntax_tree* new_ast = new syntax_tree();
        new_ast = child->copy_ast();
        
        //std::vector<std::vector<int>> vec {{1,0,0},{0,6,0},{0,0,1}};
        //add the matrix to optim.info
        
        child->new_optims.back().matrix = child->new_optims.back().matrix = get_random_matrix( shape);
        

        
        child->bounds_matrix = bounds_mat;
        child->constraint_matrix = constraint_mats.second;
        child->transformed_bounds_matrix = multiply(child->new_optims.back().matrix,bounds_mat);
        child->transformed_constraint_matrix = multiply_plus(constraint_mats.first,child->new_optims.back().matrix,constraint_mats.second);
        
        if(check_if_repeated(child->new_optims.back().matrix, matrices)) continue;
        

        child->transform_ast();

        if (!child->ast_is_legal()) {
            illegal=true;
            if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                // print deleted Ast
                child->print_previous_optims();
                std::cout << "\n-----------" << std::endl;
                child->print_new_optims();
                child->print_ast();
                child->print_isl_states();
                std::cout << "\n<illegal>\n";
            }

            if (first_time_illegal) {
                delete child;
                //iterator = children.erase(iterator);
                //if(iterator == children.end()) iterator--;
                first_time_illegal=false;
            }

            child = new_ast;
        }
        else {
                    matrices.push_back(child->new_optims.back().matrix);
                    nb_matrices++;
                    ++iterator;

                    first_time_illegal=true;
                    illegal = false;
                    if (std::atoi(read_env_var("AS_VERBOSE"))==1){
                        child->print_previous_optims();
                        std::cout << "\n-----------" << std::endl;
                        child->print_new_optims();
                        child->print_ast();
                        child->print_isl_states();
                        std::cout << "\n<legal>\n";
                    }
                    
                    int fd[2];
                    
                    // create pipe descriptors
                    pipe(fd);
                    timeout=0;
                    child_done=0;
                    cont = false;
                    std::vector<float> measurements;
                    
                    pid_t pid, ppid;
                    ppid = getpid();
                    pid = fork();

                    if (pid == -1) {
                        perror("fork failed");
                        exit(1);
                    } else if (pid == 0) {
                        measurements = exec_eval->get_measurements_matrix(*child, false, schedule_timeout);
                        int size =measurements.size();
                        float ar[measurements.size()];
                        for(int i=0;i<measurements.size();i++) ar[i]=measurements.at(i);
                        close(fd[0]);
                        write(fd[1], &size, sizeof(size));
                        
                        write(fd[1], &ar, sizeof(ar));
                        
                        close(fd[1]);
                        _exit(1);
                    }

                    // set up the signal handlers after forking so the child doesn't inherit them

                    signal(SIGALRM, alarm_handler);
                    signal(SIGCHLD, child_handler);
                    signal(SIGUSR1,sig_usr);
                    // install an alarm to be fired after TIME_LIMIT
                    
                    alarm(TIME_LIMIT);
                    
                    pause();

                    if (timeout) {
                        
                            
                        int result = waitpid(pid, NULL, WNOHANG);
                        if (result == 0) {
                            // child still running, so kill it
                            
                            
                            // Remove all the optimizations
                            exec_eval->fct->reset_schedules();
                            measurements.clear();
                            measurements.push_back(std::numeric_limits<float>::infinity());
                            // cancel any previously set alarm 
                            alarm(0); 
                            kill(pid, 9);
                            
                            
                        
                            waitpid(-1,NULL,0);
                        } else {
                            
                            int size = 0;
                            close(fd[1]);
                            read(fd[0], &size, sizeof(int));
                            float ar[size];
                            read(fd[0], &ar, size*sizeof(float));
                            for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                            close(fd[0]);
                            
                        }
                        
                        
                    }else if (child_done) {
                        
                        int size =0;
                        close(fd[1]);
                        read(fd[0], &size, sizeof(int));
                        float ar[size];
                        read(fd[0], &ar, size*sizeof(float));
                        for(int i=0;i<size;i++) measurements.push_back(ar[i]);
                        close(fd[0]);
                        
                        waitpid(-1,NULL,0);
                    }else if(cont){ 
                        alarm(0);
                        int size=0;
                        while(!child_done){}
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
                    to_be_explored.push_back(child);
                
        }
    }

    to_be_explored.resize(std::min(nb_matrices, (int)to_be_explored.size()));
    

    // Stop if we reached the maximum depth
    if (nb_explored_optims >= max_depth)
        return ;


    // Sort children from smallest evaluation to largest
    std::sort(to_be_explored.begin(), to_be_explored.end(), [](syntax_tree *a, syntax_tree *b) {
        return a->evaluation < b->evaluation;
    });

    // shuffle the children so that they are selected a random
//    std::shuffle(std::begin(children), std::end(children), rand_generator);

    // keep the top 'beam_size' children and delete the rest
    //for (int i = beam_size; i < to_be_explored.size(); ++i)
       //delete to_be_explored[i];

    //to_be_explored.resize(std::min(beam_size, (int)to_be_explored.size()));

    // Search recursively on the best children
    for (syntax_tree *child : to_be_explored)
    {
        child->search_depth = ast.search_depth ;
        
        search_save(*child, schedules_annotations, parent_trace->child_mappings[child], schedule_timeout);
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