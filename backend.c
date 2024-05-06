#include <stdio.h>
#include <stdlib.h>

#include "build_tree.h"

#define NUMREG 32
#define NUMVAR 10

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

static const char *__get_instr(const ops_t op) {
  const static struct {
    ops_t op;
    const char *instr;
  } conversion[] = {
      {ADD, "add"}, {SUB, "sub"}, {MUL, "mul"}, {DIV, "div"}, {AND, "and"},
      {OR, "or"},   {XOR, "xor"}, {SLL, "sll"}, {SRL, "srl"},
  };

  for (size_t i = 0; i < sizeof(conversion) / sizeof(conversion[0]); i++) {
    if (op == conversion[i].op) return conversion[i].instr;
  }

  return "";
}

static void __reg_reg(node_t *root, const int l_data, const int r_data) {
  int destreg;

  if (__reuse_reg(l_data) == 1) {
    destreg = l_data;
    __release_reg(r_data);
  } else if (__reuse_reg(r_data) == 1) {
    destreg = r_data;
    __release_reg(l_data);
  } else {
    destreg = assign_reg(-1);
    if (destreg == -1) {
      printf("Error: out of registers\n");
      exit(-1);
    }
    __release_reg(l_data);
    __release_reg(r_data);
  }
  printf("%s x%d, x%d, x%d\n", __get_instr((ops_t)root->data), destreg, l_data,
         r_data);
  SET_NODE(root, REG, destreg);
}

static void __unary_reg(node_t *root, const int l_data) {
  int destreg;

  if (root->data == UMINUS) {
    if (__reuse_reg(l_data)) {
      destreg = l_data;
    } else {
      destreg = assign_reg(-1);
      if (destreg == -1) {
        printf("Error: out of registers\n");
        exit(-1);
      }
      __release_reg(l_data);
    }
    printf("sub x%d, x0, x%d\n", destreg, l_data);
    SET_NODE(root, REG, destreg);
  }
}

node_t *generate_code(node_t *root) {
  node_t *left, *right;

  if ((root == NULL) || (root->type == REG)) return root;

  if (root->type == VAR) {
    SET_NODE(root, REG, vartable[root->data]);
    return root;
  }

  left = generate_code(root->left);
  right = generate_code(root->right);

  if (root->type == BINARYOP) {
    if ((left->type == REG) && (right->type == REG)) {
      __reg_reg(root, left->data, right->data);
    }
    root->right = NULL;
    free(right);
  } else if (root->type == UNARYOP) {
    if (left->type == REG) {
      __unary_reg(root, left->data);
    }
  }
  root->left = NULL;
  free(left);

  return root;
}
