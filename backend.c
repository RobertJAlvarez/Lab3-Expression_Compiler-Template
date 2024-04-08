#include <stdio.h>
#include <stdlib.h>

#include "build_tree.h"

#define SET_NODE(NODE, TYPE, DATA) \
  NODE->type = TYPE;               \
  NODE->data = DATA;

int regtable[NUMREG];  // reg[i] contains current number of uses of register i
int vartable[NUMVAR];  // var[i] contains register to which var is assigned

void init_regtable(void) {
  for (int i = 0; i < NUMREG; i++) regtable[i] = 0;
}

void init_vartable(void) {
  for (int i = 0; i < NUMVAR; i++) vartable[i] = -1;
}

static int __reuse_reg(const int reg) {
  if (regtable[reg] == 1) return 1;
  if (regtable[reg] > 1) return 0;

  fprintf(stderr, "Error: called __reuse_reg on unused register\n");

  // shouldn't happen
  return -1;
}

int assign_reg(const int var) {
  if ((var != -1) && (vartable[var] != -1)) {
    // variable is already assigned a register
    regtable[vartable[var]]++;
    return vartable[var];
  }

  // find unassigned register
  for (int i = 5; i < NUMREG; i++) {
    if (regtable[i] == 0) {
      regtable[i]++;
      if (var != -1) {
        vartable[var] = i;
      }
      return i;
    }
  }

  // out of registers
  return -1;
}

static int __release_reg(const int reg) {
  if (regtable[reg] > 0) {
    regtable[reg]--;
    return 0;
  }

  return -1;
}

void printregtable(void) {
  printf("register table -- number of uses of each register\n");

  for (int i = 0; i < NUMREG; i++)
    if (regtable[i]) printf("register: x%d, uses: %d\n", i, regtable[i]);
}

void printvartable(void) {
  printf("variable table -- register to which var is assigned\n");

  for (int i = 0; i < NUMVAR; i++)
    if (vartable[i] != -1)
      printf("variable: %c, register: x%d\n", 'a' + i, vartable[i]);
}

static void __reg_reg(node_t *root, node_t *left, node_t *right) {
  int destreg;

  if (__reuse_reg(left->data) == 1) {
    destreg = left->data;
    __release_reg(right->data);
  } else if (__reuse_reg(right->data) == 1) {
    destreg = right->data;
    __release_reg(left->data);
  } else {
    destreg = assign_reg(-1);
    if (destreg == -1) {
      printf("Error: out of registers\n");
      exit(-1);
    }
    __release_reg(left->data);
    __release_reg(right->data);
  }
  printf("%s x%d, x%d, x%d\n", optable[root->data].instr, destreg, left->data,
         right->data);
  free(left);
  free(right);
  SET_NODE(root, REG, destreg);
}

static void __unary_op(node_t *root, node_t *left) {
  int destreg;

  if (root->data == UMINUS) {
    if (left->type == REG) {
      if (__reuse_reg(left->data)) {
        destreg = left->data;
      } else {
        destreg = assign_reg(-1);
        if (destreg == -1) {
          printf("Error: out of registers\n");
          exit(-1);
        }
        __release_reg(left->data);
      }
      printf("sub x%d, x0, x%d\n", destreg, left->data);
      free(left);
      SET_NODE(root, REG, destreg);
    }
  }
}

node_t *generate_code(node_t *root) {
  node_t *left, *right;

  if (root) {
    if (root->left) left = generate_code(root->left);

    if (root->right) right = generate_code(root->right);

    // if (root->type == REG) do nothing

    if (root->type == VAR) {
      root->type = REG;
      root->data = vartable[root->data];
    } else if (root->type == BINARYOP) {
      if ((left->type == REG) && (right->type == REG)) {
        __reg_reg(root, left, right);
      }
    } else if (root->type == UNARYOP) {
      __unary_op(root, left);
    }
  }

  return root;
}
