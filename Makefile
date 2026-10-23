default_project = gnarl

projects = $(default_project) $(filter-out $(default_project),$(notdir $(wildcard src/*)))

# Board: make gnarl BOARD=heltec_v3_rfm95 (see include/boards/).
BOARD ?= heltec_v2

$(projects):
	@mk/make-project.sh $@ $(BOARD)

clean:
	rm -fr project

%:
	@echo Please specify one of these projects: $(projects)
