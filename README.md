# DSTM_P1
This is my Phd (Russian equivalent) software part. Code is difficult to understood, but someone may be interested.
# What is this repository for?
# Quick summary
Distributed software transactional memory framework (prototype 1). Tests: intset, rbtree, bank. UPC tests (for comparing): intset, rbtree, bank.
# Version
No current version, just prototype (research project)
# How do I get set up?
# Summary of set up
See src/CMakeLists.txt
# Dependencies
- LLVM 2.8

- GASNet

- DTMC (https://www.velox-project.eu/software/dtmc)

- Google gflags

- Google glog
# How to run tests
Compile test to *.bc and run with main program (--app argument)
# Publications
- http://dx.doi.org/10.14529/cmse140303 (Russian)
- http://hub.sfedu.ru/diss/announcement/3d21994d-e7ac-49c0-8cbd-293dc1cb9308/ (Phd, Russian)
