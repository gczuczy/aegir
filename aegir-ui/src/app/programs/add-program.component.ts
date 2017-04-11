import { Component, OnInit } from '@angular/core';
import { Validators, FormGroup, FormArray, FormBuilder,
	 ValidatorFn, AbstractControl } from '@angular/forms';

import { Program, ProgramMashTemp } from './program';

import { IntValidator } from '../int-validator';

@Component({
  templateUrl: './add-program.component.html',
  styleUrls: ['./add-program.component.css']
})
export class AddProgramComponent implements OnInit {
    public addProgramForm: FormGroup;

    constructor(private _fb: FormBuilder) {
    }

    ngOnInit() {
	this.addProgramForm = this._fb.group({
	    name: ['', [Validators.required, Validators.minLength(3)]],
	    endtemp: [80, Validators.required],
	    boiltime: [60, [Validators.required]],
	    mashsteps: this._fb.array([this.initMashStep(0)]),
	    hops: this._fb.array([])
	});

	// Add the endtemp validator, which updates the mashtemp validators
	this.addProgramForm.controls['endtemp'].valueChanges.subscribe({
	    next: (value) => {
		const msctrl = <FormArray>this.addProgramForm.controls['mashsteps'];

		for ( let hop of msctrl.controls ) {
		    hop.get('temp').setValidators([Validators.required, IntValidator.maxValue(value)]);
		    hop.get('temp').updateValueAndValidity();
		}
	    }
	});

	// Add the boiltime validator, which updates the hop timing validators
	this.addProgramForm.controls['boiltime'].valueChanges.subscribe({
	    next: (value) => {
		const hopctrl = <FormArray>this.addProgramForm.controls['hops'];

		for ( let hop of hopctrl.controls ) {
		    hop.get('attime').setValidators([Validators.required, IntValidator.maxValue(value)]);
		    hop.get('attime').updateValueAndValidity();
		}
	    }
	});
    }

    initMashStep(n: number) {
	let endtemp = 80;
	if ( this.addProgramForm ) {
	    endtemp = this.addProgramForm.controls['endtemp'].value
	}
	return this._fb.group({
	    order: [n],
	    temp: [42, [Validators.required, IntValidator.maxValue(endtemp)]],
	    holdtime: [15, Validators.required]
	});
    }

    addMashStep() {
	const control = <FormArray>this.addProgramForm.controls['mashsteps'];
	let ms = this.initMashStep(control.length);
	control.push(ms);
    }

    removeMashStep(n: number) {
	const control = <FormArray>this.addProgramForm.controls['mashsteps'];
	control.removeAt(n)

	// and fix the order fields
	const msctrl = <FormArray>this.addProgramForm.controls['mashsteps'];

	let i = 0;
	for ( let hop of msctrl.controls ) {
	    hop.get('order').setValue(i);
	    i = i + 1;
	}
    }

    initHop() {
	const boiltime = this.addProgramForm.controls['boiltime']
	return this._fb.group({
	    attime: [0, [Validators.required, IntValidator.maxValue(boiltime.value)]],
	    name: ['', Validators.required],
	    quantity: [0, [Validators.required, IntValidator.minValue(1)]]
	});
    }

    addHop() {
	const control = <FormArray>this.addProgramForm.controls['hops'];
	control.push(this.initHop());
    }

    removeHop(i: number) {
	const control = <FormArray>this.addProgramForm.controls['hops'];
	control.removeAt(i)
    }

    save(model: FormGroup) {
	console.log('save ', model);
	console.log('save ', model.value);
    }
}
