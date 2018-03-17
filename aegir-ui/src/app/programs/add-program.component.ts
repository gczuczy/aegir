import { Component, OnInit } from '@angular/core';
import { Validators, FormGroup, FormArray, FormBuilder,
	 ValidatorFn, AbstractControl } from '@angular/forms';
import { Router } from '@angular/router';

import { Program, ProgramMashTemp } from './program';

import { IntValidator } from '../int-validator';
import { ApiService } from '../api.service';

@Component({
  templateUrl: './add-program.component.html',
  styleUrls: ['./add-program.component.css']
})
export class AddProgramComponent implements OnInit {
    public addProgramForm: FormGroup;

    private minmashtemp = 37;
    private maxmashtemp = 80;
    public hasmash:boolean;
    public hasboil:boolean;

    public errors: string[];

    constructor(private _fb: FormBuilder,
		private api: ApiService,
		private router: Router) {
    }

    ngOnInit() {
	this.hasmash = true;
	this.hasboil = true;
	this.addProgramForm = this._fb.group({
	    name: ['', [Validators.required, Validators.minLength(3)]],
	    starttemp: [this.minmashtemp, [Validators.required, IntValidator.minValue(25), IntValidator.maxValue(80)]],
	    endtemp: [this.maxmashtemp, [Validators.required, IntValidator.minValue(37), IntValidator.maxValue(90)]],
	    boiltime: [60, [Validators.required, IntValidator.minValue(30), IntValidator.maxValue(300)]],
	    mashsteps: this._fb.array([this.initMashStep(0)]),
	    hasmash: [this.hasmash],
	    hasboil: [this.hasboil],
	    hops: this._fb.array([])
	}, {
	    validator: this.formValidator.bind(this)
	});

	// reset the error array
	this.errors = null;

	// Add the starttemp validator, which updates the mashtemp validators
	this.addProgramForm.controls['starttemp'].valueChanges.subscribe({
	    next: (value) => {
		const endctrl = <FormArray>this.addProgramForm.controls['endtemp'];
		let endval = endctrl.value

		// set the mintemp's max
		if ( value != this.minmashtemp ) {
		    endctrl.setValidators([Validators.required, IntValidator.minValue(value), IntValidator.maxValue(90)]);
		    endctrl.updateValueAndValidity();
		    this.minmashtemp = value;
		}
	    }
	});

	// Add the endtemp validator, which updates the mashtemp validators
	this.addProgramForm.controls['endtemp'].valueChanges.subscribe({
	    next: (value) => {
		const startctrl = <FormArray>this.addProgramForm.controls['starttemp'];
		let startval = startctrl.value

		// set the mintemp's max
		if ( value != this.maxmashtemp ) {
		    startctrl.setValidators([Validators.required, IntValidator.minValue(25), IntValidator.maxValue(value)]);
		    startctrl.updateValueAndValidity();
		    this.maxmashtemp = value;
		}
	    }
	});

	// Add the boiltime validator, which updates the hop timing validators
	this.addProgramForm.controls['boiltime'].valueChanges.subscribe({
	    next: (value) => {
		const hopctrl = <FormArray>this.addProgramForm.controls['hops'];

		for ( let hop of hopctrl.controls ) {
		    hop.get('attime').setValidators([Validators.required, IntValidator.minValue(0), IntValidator.maxValue(value)]);
		    hop.get('attime').updateValueAndValidity();
		}
	    }
	});
    }

    formValidator(form: FormGroup): {[key: string]: any} {
	//console.log('formValidator called', form);
	if ( !this.addProgramForm ) return null;

	let startval = <FormArray>this.addProgramForm.controls['starttemp'].value;
	let endval = <FormArray>this.addProgramForm.controls['endtemp'].value;

	const msctrl = <FormArray>this.addProgramForm.controls['mashsteps'];
	let tempsteps: Array<number> = [];
	// collect the values
	let nosteps = 0;
	for ( let step of msctrl.controls ) {
	    tempsteps.push(step.get('temp').value);
	    ++nosteps;
	}

	let absmin = startval;
	let absmax = endval;
	let i = 0;
	/*
	console.log('absmin:', absmin, ' absmax:', absmax,
		    ' ctrlen:', msctrl.controls.length,
		    ' tempsteps:', tempsteps);
	*/
	for ( let step of msctrl.controls ) {
	    let min = <number>(i==0 ? absmin : tempsteps[i-1]);
	    let max = <number>(i+1==msctrl.controls.length ? absmax : tempsteps[i+1]);
	    let stepval = step.get('temp').value;
		/*
	    console.log('i:', i, ' Min:', min, ' max:', max, ' val:', stepval,
			' status:', step.get('temp').status);
		*/
	    step.get('temp').setValidators([Validators.required, IntValidator.minValue(min+1),
					    IntValidator.maxValue(max-1)]);
	    if ( (stepval <= min || stepval >= max) || step.get('temp').invalid ) {
		step.get('temp').markAsTouched();
	    }
	    if ( (stepval > min && stepval < max && step.get('temp').invalid) ||
		 (stepval <= min || stepval >= max) && step.get('temp').valid ) {
		step.get('temp').updateValueAndValidity();
	    }
	    i += 1;
	}

	let hasboil = <FormArray>this.addProgramForm.controls['hasboil'].value;
	let hasmash = <FormArray>this.addProgramForm.controls['hasmash'].value;
	if ( !hasboil && !hasmash ) return {'mashboil': 'Either mash or boil is needed'};

	if ( hasboil ) {
	    const hops = <FormArray>this.addProgramForm.controls['hops'];
	    if ( hops.length == 0 )
		return {'needhops': 'Need hops when boiling'}
	}

	if ( hasmash ) {
	    const steps = <FormArray>this.addProgramForm.controls['mashsteps'];
	    if ( steps.length == 0 )
		return {'needmashsteps': 'Need at least 1 mash steps'}
	}

	return null;
    }

    initMashStep(n: number) {
	let endtemp = 80;
	if ( this.addProgramForm ) {
	    endtemp = this.addProgramForm.controls['endtemp'].value
	}
	return this._fb.group({
	    order: [n],
	    temp: [42, [Validators.required, IntValidator.minValue(20), IntValidator.maxValue(endtemp)]],
	    holdtime: [15, [Validators.required]]
	});
    }

    public addMashStep() {
	const control = <FormArray>this.addProgramForm.controls['mashsteps'];
	let ms = this.initMashStep(control.length);
	control.push(ms);
    }

    public removeMashStep(n: number) {
	const control = <FormArray>this.addProgramForm.controls['mashsteps'];
	control.removeAt(n)

	// and fix the order fields
	const msctrl = <FormArray>this.addProgramForm.controls['mashsteps'];

	let i = 0;
	for ( let step of msctrl.controls ) {
	    step.get('order').setValue(i);
	    i = i + 1;
	}
    }

    initHop() {
	const boiltime = this.addProgramForm.controls['boiltime']
	return this._fb.group({
	    attime: [0, [Validators.required, IntValidator.minValue(0), IntValidator.maxValue(boiltime.value)]],
	    name: ['', [Validators.required, Validators.minLength(3)]],
	    quantity: [0, [Validators.required, IntValidator.minValue(1)]]
	});
    }

    public addHop() {
	const control = <FormArray>this.addProgramForm.controls['hops'];
	control.push(this.initHop());
    }

    public removeHop(i: number) {
	const control = <FormArray>this.addProgramForm.controls['hops'];
	control.removeAt(i)
    }

    onHasmashChange(event) {
	this.hasmash = event;
    }

    onHasboilChange(event) {
	this.hasboil = event;
    }

    public save(model: FormGroup) {
	/*
	console.log('save ', model);
	console.log('save ', model.value);
	*/
	this.api.addProgram(model.value).subscribe(
	    resp => {
		let progid = resp['data']['progid'];
		this.errors = null;
		this.api.announceUpdate();
		//console.log('Program Added with response: ', resp);
		this.router.navigate(['programs', progid, 'view']);
	    },
	    err => {
		let errobj = err.json();
		//console.log('Error obj: ', errobj);
		this.errors = errobj['errors'];
	    }
	);
    }
}
