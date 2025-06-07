BIN_DIR=../../fvm
INST_DIR=usr/bin/fvm
BUILD_FILES=build_files

build:
	if [ -d "fvm" ]; then rm -fr fvm; fi

	mkdir -p fvm/$(INST_DIR)/hardware

	touch fvm/$(INST_DIR)/hardware/rom fvm/$(INST_DIR)/hardware/disk

	cp $(BIN_DIR)/fvma fvm/$(INST_DIR)
	cp $(BIN_DIR)/fvmr fvm/$(INST_DIR)

	sudo chown root:root fvm/$(INST_DIR)/fvma
	sudo chown root:root fvm/$(INST_DIR)/fvmr

	sudo chmod u=rwx,g=rx,o=rx fvm/$(INST_DIR)/fvma
	sudo chmod u=rwx,g=rx,o=rx fvm/$(INST_DIR)/fvmr

	cp $(BUILD_FILES)/fvma fvm/usr/bin
	cp $(BUILD_FILES)/fvmr fvm/usr/bin
	cp $(BUILD_FILES)/fvmlr fvm/usr/bin
	cp $(BUILD_FILES)/fvmld fvm/usr/bin
	cp $(BUILD_FILES)/fvmalr fvm/usr/bin

	sudo chmod u=rwx,g=rx,o=rx fvm/usr/bin/fvma
	sudo chmod u=rwx,g=rx,o=rx fvm/usr/bin/fvmr
	sudo chmod u=rwx,g=rx,o=rx fvm/usr/bin/fvmlr
	sudo chmod u=rwx,g=rx,o=rx fvm/usr/bin/fvmld
	sudo chmod u=rwx,g=rx,o=rx fvm/usr/bin/fvmalr

	cp -r $(BUILD_FILES)/DEBIAN fvm

	dpkg-deb -b fvm
