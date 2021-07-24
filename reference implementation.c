#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
/* functions */
#define MAX_LINE_CHARS 1025
#define BLACK 'B'
#define RED 'R'
#define LEFT 0
#define RIGHT 1

/* data structures */
typedef struct rb_node{
    char color;
    int version;
    int line_no;
    char *line_text;
    struct rb_node *left;
    struct rb_node *right;
}
rb_node_t;

/* String management */
char * duplicate_string(char* string){
    if(string != NULL){
       char* copy = malloc(MAX_LINE_CHARS);
       return strncpy(copy,string,1024);
    }
    return NULL;
}

void free_string(char* string){
    free(string);
    return;
}

/* Creates a red node, setting line_no and allocating fresh string
 * sets the version to the current_version */
rb_node_t * create_node(int line_no, char* string){
    rb_node_t * x = malloc(sizeof(rb_node_t));
    x->color = RED;
    x->version = -1;
    x->line_no = line_no;
    x->line_text = duplicate_string(string);
    x->left  = NULL;
    x->right = NULL;
    return x;
}

rb_node_t * duplicate_node(rb_node_t *node){
    if (node == NULL) { return NULL; }
    rb_node_t * x = malloc(sizeof(rb_node_t));
    x->color = node->color;
    x->version = -1;
    x->line_no = node->line_no;
    x->line_text = duplicate_string(node->line_text);
    x->left  = node->left;
    x->right = node->right;
    return x;
}

void free_node(rb_node_t * node){
       free(node->line_text);
       free(node);
    return;
}

/* Version management */
/* Global Text representation singleton*/
#define NUM_VERSIONS_PER_SLAB 8192
struct {
    rb_node_t ** versions; /* holds a pointer to the tree root*/
    int current_version;
    int last_version;
    int max_versions;
} text_state = {.versions = NULL,
                .current_version = -1,
                .last_version = -1,
                .max_versions = 0 };

rb_node_t * update_node_lastver(rb_node_t *node){
    if (node == NULL) {
        return NULL;
    }
    if(node->version == text_state.last_version) {return node;}
    rb_node_t * x;
    x = duplicate_node(node);
    x->version = text_state.last_version;
    return x;
}

void delete_version(rb_node_t **root,int version_to_delete){
    if ((*root) != NULL){
        if ( ((*root)->left != NULL) &&
            ((*root)->left->version == version_to_delete)) {
           delete_version( &(*root)->left,version_to_delete);
        }
        if ( ((*root)->right != NULL) &&
             ((*root)->right->version == version_to_delete)) {
           delete_version( &(*root)->right,version_to_delete);
        }
        if ((*root)->version == version_to_delete) {
           free_node((*root));
           *root=NULL;
        }
    }
    return;
}

void delete_version_fullvisit(rb_node_t **root,int version_to_delete){
    if ((*root) != NULL){
           delete_version_fullvisit( &(*root)->left,version_to_delete);
           delete_version_fullvisit( &(*root)->right,version_to_delete);
        if ((*root)->version == version_to_delete) {
           free_node((*root));
           *root=NULL;
        }
    }
    return;
}


/* creates new version after the *current* one. Avoids history divergence*/
void create_new_version(void){
    /* While timeline divergences are present, remove them */
    while(text_state.last_version > text_state.current_version){
           delete_version(text_state.versions + text_state.last_version,
                          text_state.last_version);
           text_state.last_version--;
    }
    if (text_state.last_version < text_state.current_version) {
        printf("Is that you, Marty McFly?\n");
    }
    text_state.current_version++;
    text_state.last_version++;
    if (text_state.last_version >= text_state.max_versions ){
        text_state.max_versions += NUM_VERSIONS_PER_SLAB;
        text_state.versions = realloc(text_state.versions,
                                      sizeof(rb_node_t*)*text_state.max_versions);
        memset(text_state.versions+text_state.max_versions-NUM_VERSIONS_PER_SLAB,
               0,sizeof(rb_node_t*)*NUM_VERSIONS_PER_SLAB);
    }
    if (text_state.last_version > 0){
       if (text_state.versions[text_state.last_version-1] != NULL) {
         text_state.versions[text_state.last_version] = duplicate_node(text_state.versions[text_state.last_version-1]);
         text_state.versions[text_state.last_version]->version = text_state.versions[text_state.last_version-1]->version + 1;
       } else {
           text_state.versions[text_state.last_version] = NULL;
       }
    }
  return;
}

rb_node_t * single_rotate_ver(rb_node_t * root, int direction) {
    rb_node_t *save;
    root = update_node_lastver(root);
    if (direction == LEFT) {
      root->right = update_node_lastver(root->right);
      save = root->right;
      root->right = save->left;
      save->left = root;
    } else {
      root->left = update_node_lastver(root->left);
      save = root->left;
      root->left = save->right;
      save->right = root;
    }
    root->color = RED;
    save->color = BLACK;
    return save;
}

rb_node_t * double_rotate_ver(rb_node_t *root, int direction) {
    root = update_node_lastver(root);
    if(direction == LEFT) {
       /* single rotate takes care of duplicating its root */
       root->right = single_rotate_ver(root->right, RIGHT);
       return single_rotate_ver(root, LEFT);
    } else {
       root->left = single_rotate_ver(root->left, LEFT);
       return single_rotate_ver(root, RIGHT);
    }
    /* should never get here */
    return NULL;
}

int is_red(rb_node_t * to_test){
    return (to_test != NULL) && (to_test->color == RED);
}

rb_node_t * topdown_rb_insert_last_version(rb_node_t **root_ptr, int line_no, char* string){
    rb_node_t* current;
    if (*root_ptr == NULL) {
        /* Empty tree case */
        *root_ptr = create_node(line_no,string);
        (*root_ptr)->version = text_state.last_version;
        current = *root_ptr;
    }
    else {
        rb_node_t head = { 0 }; /* False tree root ancestor */
        head.version = text_state.last_version;

        rb_node_t *gramps, *grandgramps;     /* Grandparent & parent */
        rb_node_t *par;     /* Iterator & parent */
        int direction = LEFT, gramps_to_par_direction = LEFT;

        /* Set up helpers */
        grandgramps = &head;
        gramps = par = NULL;

        grandgramps->right = *root_ptr;
        current = *root_ptr;
        assert((*root_ptr)->version == text_state.last_version);

        /* Search down the tree */
        for (;;) {
            if (current == NULL) {
                /* We got to the insertion point, and there was no such line,
                 * insert it. */
                current = create_node(line_no,string);
                current->version = text_state.last_version;
                if(direction == LEFT) {
                   par->left = current;
                } else {
                   par->right = current;
                }
            }
            else if ( is_red(current->left) && is_red(current->right)) {
                /* Color flip, push down blackness */
                rb_node_t * new_cur;
                new_cur = update_node_lastver(current);
                if (par !=NULL) {
                    if(par->left == current){
                        par->left = new_cur;
                    } else {
                        par->right = new_cur;
                    }
                } else {
                    *root_ptr = new_cur;
                }
                current = new_cur;
                current->color = RED;
                current->left = update_node_lastver(current->left);
                current->left->color = BLACK;
                current->right = update_node_lastver(current->right);
                current->right->color = BLACK;
            }
            /* Fix red violation which has been pushed down */
            if (is_red(current) && is_red(par)) {
                int is_gramps_right_son = (grandgramps->right == gramps);
                int am_i_outer_child = ( gramps_to_par_direction == LEFT ?
                                               (current == par->left) :
                                               (current == par->right));
                /* I will modify grandgramps reference to grampa */
                grandgramps = update_node_lastver(grandgramps);
                if(is_gramps_right_son){
                   if (am_i_outer_child) { /* Case 3 of CRL */
                       grandgramps->right = single_rotate_ver(gramps, !gramps_to_par_direction);
                   }
                   else { /* Case 2 of CRL */
                       grandgramps->right = double_rotate_ver(gramps, !gramps_to_par_direction);
                   }
                } else {
                   if (am_i_outer_child) {
                       grandgramps->left = single_rotate_ver(gramps, !gramps_to_par_direction);
                   }
                   else {
                       grandgramps->left = double_rotate_ver(gramps, !gramps_to_par_direction);
                   }
                }
            }

            /* We are at the insertion point. We either inserted before, or
             * we are going to update an existing node, bumping the version*/
            if (current->line_no == line_no) {
                /* before completing, if existing with a different value,
                 * update found node */
                if  (strncmp(current->line_text,string,MAX_LINE_CHARS) != 0) {
                    /* if node is outdated, refresh node before changing
                     * content*/
                    if (current->version != text_state.last_version) {
                       rb_node_t * updated_cur;
                       updated_cur = update_node_lastver(current);
                       if (par !=NULL) {
                           if(par->left == current){
                               par->left = updated_cur;
                           } else {
                               par->right = updated_cur;
                           }
                       } else {
                           *root_ptr = updated_cur;
                       }
                       current = updated_cur;
                    }
                    free_string(current->line_text);
                    current->line_text = strndup(string,MAX_LINE_CHARS);
                }
                break;
            }
            gramps_to_par_direction = direction;
            direction = current->line_no < line_no;
            /* Update helpers */
            if (gramps != NULL) {
                grandgramps = gramps;
            }
            gramps = par;
            par = current;
            if(direction == LEFT) {
               if (current->left !=NULL) {
                   current->left = update_node_lastver(current->left);
               }
               current = current->left;
            } else {
               if (current->right !=NULL) {
                   current->right = update_node_lastver(current->right);
               }
               current = current->right;
            }
        }
        /* Update root */
        *root_ptr = head.right;
    }
    /* Make root black */
    (*root_ptr)->color = BLACK;
    return current;
}

void topdown_rb_delete_last_version(rb_node_t **root_ptr, int line_no){
    if (*root_ptr != NULL) {
        rb_node_t head = { 0 }; /* mockup tree root to avoid special cases */
        head.version = text_state.last_version;
        rb_node_t *to_delete, *parent_of_to_delete, *gramps_of_to_delete; /* Helpers */
        rb_node_t *node_of_value_to_delete = NULL;  /* Found item */
        int direction = RIGHT;

        /* Set up helpers */
        to_delete = &head;
        parent_of_to_delete = NULL;
        gramps_of_to_delete = NULL;

        /*starting point is set as to_delete's right child. Update root if needed*/
        *root_ptr = update_node_lastver(*root_ptr);
        to_delete->right = *root_ptr;
        to_delete->left = NULL;

        /* Child of to_delete where I will descend */
        rb_node_t * descent_dir = to_delete->right;
        rb_node_t * other_dir = to_delete->left;

        /* While looking for the node, push a red down */
        /* Search for either the node (if without a child) or its predecessor */
        while (descent_dir != NULL) {
            int parent_to_me_dir = direction;
            /* Update tracking pointers */
            gramps_of_to_delete = parent_of_to_delete;
            parent_of_to_delete = to_delete;
            /* freshly involved node in descent may need to be updated, other
             * are already up to date due to loop invariant. */
            descent_dir = update_node_lastver(descent_dir);
            /* Rebind new descendant. */
            if(direction == LEFT) {
                to_delete->left = descent_dir;
            } else {
                to_delete->right = descent_dir;
            }
            /* Actually descend. */
            to_delete = descent_dir;
            direction = (to_delete->line_no < line_no);
            if(direction == LEFT) {
                descent_dir = to_delete->left;
                other_dir = to_delete->right;
            } else {
                descent_dir = to_delete->right;
                other_dir   = to_delete->left;
            }
            /* If found node with value to delete, save found node. Saved node
             * is already in the up-to-date tree. */
            if (to_delete->line_no == line_no) {
                node_of_value_to_delete = to_delete;
            }

            /* make the node in descent_dir red if possible (i.e., I'm not red)
             * and needed (it's black) */
         if ( !is_red(to_delete) && !is_red(descent_dir) ) {
             if (is_red(other_dir)) {
                   /* the non-descent direction is red, rotate on me pushing */
                   /* down red from other direction child */
                   if(parent_to_me_dir == LEFT){
                       parent_of_to_delete->left = single_rotate_ver(to_delete, direction);
                       parent_of_to_delete = parent_of_to_delete->left;
                   } else {
                       parent_of_to_delete->right = single_rotate_ver(to_delete, direction);
                       parent_of_to_delete = parent_of_to_delete->right;
                   }
             } else { /* tough luck, I am black, with both black children */
                      /* look at my sibling, he'll be involved, update*/
                    rb_node_t* to_delete_sibling;
                    if(parent_to_me_dir == LEFT) {
                        parent_of_to_delete->right = update_node_lastver(parent_of_to_delete->right);
                        to_delete_sibling = parent_of_to_delete->right;
                    } else {
                        parent_of_to_delete->left = update_node_lastver(parent_of_to_delete->left);
                        to_delete_sibling = parent_of_to_delete->left;
                    }
                 /* if I have a sibling */
                 if (to_delete_sibling != NULL) {
                        /* if sibling has black sons, I can blacken my father and
                         * redden me and my sibling w/o violations */
                     if (!is_red(to_delete_sibling->right) && !is_red(to_delete_sibling->left)) {
                         parent_of_to_delete->color = BLACK;
                         to_delete_sibling->color = RED;
                         to_delete->color = RED;
                     } else { /* ew, tough luck. I cannot redden my sibling
                               * act on sibling and nephews */
                            int dad_is_right_son = gramps_of_to_delete->right == parent_of_to_delete;
                            rb_node_t * outer_nephew, *inner_nephew, *new_dad;
                            to_delete_sibling->right = update_node_lastver(to_delete_sibling->right);
                            to_delete_sibling->left  = update_node_lastver(to_delete_sibling->left);
                            if(parent_to_me_dir == RIGHT) {
                                inner_nephew = to_delete_sibling->right;
                                outer_nephew = to_delete_sibling->left;
                            } else {
                                inner_nephew = to_delete_sibling->left;
                                outer_nephew = to_delete_sibling->right;
                            }
                            assert(gramps_of_to_delete->version == text_state.last_version);
                            if(dad_is_right_son) {
                                if(is_red(inner_nephew)){ /* Case 3 */
                                   gramps_of_to_delete->right = double_rotate_ver(parent_of_to_delete,parent_to_me_dir);
                                } else if(is_red(outer_nephew)) { /* Case 4 */
                                   gramps_of_to_delete->right = single_rotate_ver(parent_of_to_delete,parent_to_me_dir);
                                }
                                new_dad = gramps_of_to_delete->right;
                            } else {
                                if(is_red(inner_nephew)){
                                       gramps_of_to_delete->left = double_rotate_ver(parent_of_to_delete,parent_to_me_dir);
                                } else if(is_red(outer_nephew)) {
                                       gramps_of_to_delete->left = single_rotate_ver(parent_of_to_delete,parent_to_me_dir);
                                }
                                new_dad = gramps_of_to_delete->left;
                            }
                            new_dad->color = RED;
                            new_dad->left->color = BLACK;
                            new_dad->right->color = BLACK;
                            to_delete->color = RED;
                     }
                 }
             }
          } /* End of pushing red down */
        }  /* found a spliceable node, either value, or predecessor */

        /* If node to delete is found, copy line_no on predecessor if needed, and
         * deallocate */
        if (node_of_value_to_delete != NULL) {
            if (node_of_value_to_delete != to_delete) {
               assert(node_of_value_to_delete->version  == text_state.last_version);
               node_of_value_to_delete->line_no = to_delete->line_no;
               strncpy(node_of_value_to_delete->line_text,to_delete->line_text,1024);
            }
            int direction = (parent_of_to_delete->right == to_delete);
            rb_node_t * subtree_to_graft;
            if (to_delete->left == NULL) {
                subtree_to_graft = to_delete->right;
            } else {
                subtree_to_graft = to_delete->left;
            }
            if(direction == RIGHT) {
                parent_of_to_delete->right = subtree_to_graft ;
            } else {
                parent_of_to_delete->left  = subtree_to_graft ;
            }
            free_node(to_delete);
        }

        *root_ptr = head.right;
        if (*root_ptr != NULL) {
            (*root_ptr)->color = BLACK;
        }
    }
    return;
}

/*************************** UNDO AND REDO ************************************/
void undo(int versions){
    text_state.current_version = text_state.current_version - versions;
    if (text_state.current_version < 0){
        text_state.current_version = 0;
    }
    return;
}

void redo(int versions){
    text_state.current_version = text_state.current_version + versions;
    if (text_state.current_version > text_state.last_version){
        text_state.current_version = text_state.last_version;
    }
    return;
}

/****************************** CHANGE ****************************************/

void change(int first, int last, FILE* fpin){
    create_new_version();
//     printf(" Changing from line %d, to line %d \n",first,last);
    char linebuffer[1025] = {0};
    int lines_to_insert = last-first+1;
    for(int offset=0; offset < lines_to_insert; offset++){
       fgets(linebuffer,1024,fpin);
       linebuffer[strcspn(linebuffer, "\n\r")] = 0;
//        fprintf(stderr,"inserting line %d, --%s--\n",first+offset,linebuffer);
       topdown_rb_insert_last_version(&text_state.versions[text_state.last_version],
                                      first+offset,
                                      linebuffer);
    }
    fgets(linebuffer,1024,fpin);
    if(linebuffer[0] != '.') {
        printf("missing single line termination!\n");
    }
    return;
}

/****************************** DELETE ****************************************/

/* Pre: the root node is always at the last version  */
void scroll_lines(rb_node_t* root, int scroll_amt, int first_line_to_scroll){
    if(root == NULL) return;

    if ( (root->left != NULL) &&
         (root->left->version !=text_state.last_version) &&
         (root->left->line_no >= first_line_to_scroll)) {
        /* version update of child, so that I also fix parent pointer */
        root->left = update_node_lastver(root->left);
    }
    scroll_lines(root->left,scroll_amt,first_line_to_scroll);

    if(root->line_no >= first_line_to_scroll){
        root->line_no -= scroll_amt;
    }

    if ((root->right != NULL) &&
        (root->right->version !=text_state.last_version) &&
        (root->right->line_no >= first_line_to_scroll)){
        /* version update of child, so that I also fix parent pointer */
        root->right = update_node_lastver(root->right);
    }
    scroll_lines(root->right,scroll_amt,first_line_to_scroll);
    return;

}

void delete(int first, int last){
    create_new_version();
       for(int line_to_delete=first;line_to_delete <= last; line_to_delete++){
          topdown_rb_delete_last_version(&text_state.versions[text_state.last_version],
                                         line_to_delete);
       }
       scroll_lines(text_state.versions[text_state.last_version], last-first+1,last+1);
    return;
}

/******************************* PRINT ****************************************/

struct {
    rb_node_t ** nodes_to_print;
    int last_filled;
    int capacity;
} to_print_collection = {.nodes_to_print = NULL,
                         .last_filled = -1,
                         .capacity = 0 };

void add_line_to_collection(rb_node_t* node){
    if (to_print_collection.last_filled == to_print_collection.capacity-1){
        to_print_collection.nodes_to_print= realloc(to_print_collection.nodes_to_print,(to_print_collection.capacity+8192)*sizeof(rb_node_t*));
        to_print_collection.capacity += 8192;
    }
    to_print_collection.last_filled++;
    to_print_collection.nodes_to_print[to_print_collection.last_filled] = node;
}

void collect_lines_to_print(rb_node_t * root,int first, int last){
    if(root == NULL) return;
    collect_lines_to_print(root->left,first,last);
    if ( (root->line_no >= first) && (root->line_no <= last) ) {
        add_line_to_collection(root);
    }
    collect_lines_to_print(root->right,first,last);
    return;
}

void print_lines(int first_to_print, int last_to_print, FILE* fpout){

    to_print_collection.last_filled = -1;
    if (text_state.current_version>=0) {
    collect_lines_to_print(text_state.versions[text_state.current_version],
                           first_to_print,
                           last_to_print);
    }

    int last_printed_line = first_to_print;
    if ((to_print_collection.last_filled >=0) && (text_state.current_version>=0)){
       for(int line = 0; line <= to_print_collection.last_filled; line++ ){
          for(int leading_dots = 0;
              leading_dots < to_print_collection.nodes_to_print[line]->line_no-last_printed_line-1;
              leading_dots++){
                 printf(".\n");
          }
          printf("%s\n",to_print_collection.nodes_to_print[line]->line_text);

          last_printed_line = to_print_collection.nodes_to_print[line]->line_no;
       }
       for(int leading_dots = 0;
           leading_dots < last_to_print-last_printed_line;
           leading_dots++){
              printf(".\n");
       }
    } else {
        for (int dots =0; dots < last_to_print-first_to_print+1 ;dots++){
            printf(".\n");
        }
    }
    return;
}


void parse(void){
    char command ='x';
    char linebuf[1024];
    char *token;
    int first_line=0,last_line=0;
    while(1){
        memset(linebuf,0,1024);
        fgets(linebuf,1024,stdin);
        linebuf[strcspn(linebuf, "\n\r")] = 0;
        token = strtok(linebuf, ",");

        command = 'x';
        /* actual command*/
        if ((token[0] >= '0') && (token[0] <= '9')){
           int last_token_char = token[strlen(token)-1];
           if ((last_token_char >= '0') && (last_token_char <= '9')){
               first_line = atoi(token);
               token = strtok(NULL, ",");
               command = token[strlen(token)-1];
               token[strlen(token)-1] = 0;
               last_line = atoi(token);
           } else {
               command=last_token_char;
               token[strlen(token)-1] = 0;
               first_line = atoi(token);
           }
        } else if (*token == 'q') {
                return;
        }
        switch (command){
            case 'c': change(first_line, last_line, stdin);
                break;
            case 'd': delete(first_line,last_line);
                break;
            case 'p': print_lines(first_line,last_line, stdout);
                break;
            case 'u': undo(first_line);
                break;
            case 'r': redo(first_line);
                break;
        }
    }

}


int main(int argc,char* argv[]){

   parse();

   while (text_state.last_version >=0){
     delete_version(text_state.versions+text_state.last_version,
                              text_state.last_version);
     text_state.current_version--;
     text_state.last_version--;
   }
   free(text_state.versions);
   free(to_print_collection.nodes_to_print);
   return 0;
}
