CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -Wno-sign-compare

TEST_DIR = test
SRC_DIR = trie
BIN_DIR = bin


TARGETS = $(BIN_DIR)/trie_test1 $(BIN_DIR)/trie_test2 $(BIN_DIR)/trie_test3 \
          $(BIN_DIR)/trie_test4 $(BIN_DIR)/trie_noncopy_test $(BIN_DIR)/trie_store_test1 \
          $(BIN_DIR)/trie_store_test2 $(BIN_DIR)/trie_store_test3 \
          $(BIN_DIR)/trie_store_noncopy_test $(BIN_DIR)/trie_store_correctness_test \


all: $(BIN_DIR) $(TARGETS)


$(BIN_DIR):
	mkdir -p $@


$(BIN_DIR)/%: $(TEST_DIR)/%.cpp $(SRC_DIR)/src.hpp
	$(CXX) $(CXXFLAGS) -o $@ $<


clean:
	rm -rf $(BIN_DIR)

# Thanks Renhao Zhang for giving advice