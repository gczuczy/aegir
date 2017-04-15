import { Component, OnInit } from '@angular/core';
import { Validators, FormGroup, FormArray, FormBuilder,
	 ValidatorFn, AbstractControl } from '@angular/forms';
import { ActivatedRoute, Params, Router } from '@angular/router';
import 'rxjs/add/operator/switchMap';

import { IntValidator } from '../int-validator';
import { Program } from './program';
import { ApiService } from '../api.service';

@Component({
  templateUrl: './edit-program.component.html',
  styleUrls: ['./edit-program.component.css']
})
export class EditProgramComponent implements OnInit {
    public editProgramForm: FormGroup;

    private minmashtemp = 37;
    private maxmashtemp = 80;

    public errors: string[];

    public program: Program;

    constructor(private _fb: FormBuilder,
		private api: ApiService,
		private route: ActivatedRoute,
		private router: Router) {
    }

    ngOnInit() {
	//debugger;
	this.route.params
	    .switchMap((params: Params) => this.api.getProgram(params['id']))
	    .subscribe(prog => {
		this.program = prog['data'];
		this.editProgramForm = this._fb.group({
		    id: [this.program['id']],
		    name: [this.program['name'], [Validators.required, Validators.minLength(3), Validators.maxLength(32)]],
		    starttemp: [this.program['starttemp'], [Validators.required, IntValidator.minValue(25), IntValidator.maxValue(80)]],
		    endtemp: [this.program['endtemp'], [Validators.required, IntValidator.minValue(37), IntValidator.maxValue(90)]],
		    boiltime: [this.program['boiltime'], [Validators.required, IntValidator.minValue(30), IntValidator.maxValue(300)]],
		    mashsteps: this._fb.array(this.initMashSteps(this.program['mashsteps'])),
		    hops: this._fb.array(this.initHops(this.program['hops']))
		}, {
		    validator: this.formValidator.bind(this)
		});
		this.defineValidators();
	    });

	// reset the error array
	this.errors = null;

    }

    defineValidators() {
	// Add the starttemp validator, which updates the mashtemp validators
	this.editProgramForm.controls['starttemp'].valueChanges.subscribe({
	    next: (value) => {
		const endctrl = <FormArray>this.editProgramForm.controls['endtemp'];
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
	this.editProgramForm.controls['endtemp'].valueChanges.subscribe({
	    next: (value) => {
		const startctrl = <FormArray>this.editProgramForm.controls['starttemp'];
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
	this.editProgramForm.controls['boiltime'].valueChanges.subscribe({
	    next: (value) => {
		const hopctrl = <FormArray>this.editProgramForm.controls['hops'];

		for ( let hop of hopctrl.controls ) {
		    hop.get('attime').setValidators([Validators.required, IntValidator.minValue(0), IntValidator.maxValue(value)]);
		    hop.get('attime').updateValueAndValidity();
		}
	    }
	});
    }

    formValidator(form: FormGroup): {[key: string]: any} {
	if ( !this.editProgramForm ) return null;

	let startval = <FormArray>this.editProgramForm.controls['starttemp'].value;
	let endval = <FormArray>this.editProgramForm.controls['endtemp'].value;

	const msctrl = <FormArray>this.editProgramForm.controls['mashsteps'];
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
	return null;
    }

    initMashStep(n: number) {
	let endtemp = 80;
	if ( this.editProgramForm ) {
	    endtemp = this.editProgramForm.controls['endtemp'].value
	}
	return this._fb.group({
	    order: [n],
	    temp: [42, [Validators.required, IntValidator.minValue(20), IntValidator.maxValue(endtemp)]],
	    holdtime: [15, [Validators.required]]
	});
    }

    initMashSteps(steps: any[]) {
	let endtemp = 80;
	if ( this.editProgramForm ) {
	    endtemp = this.editProgramForm.controls['endtemp'].value
	}
	let fbsteps = [];
	for ( let step of steps ) {
	    fbsteps.push(this._fb.group({
		order: [step['orderno']],
		temp: [step['temperature'], [Validators.required, IntValidator.minValue(20), IntValidator.maxValue(endtemp)]],
		holdtime: [step['holdtime'], [Validators.required]]
	    }));
	}
	return fbsteps;
    }

    addMashStep() {
	const control = <FormArray>this.editProgramForm.controls['mashsteps'];
	let ms = this.initMashStep(control.length);
	control.push(ms);
    }

    removeMashStep(n: number) {
	const control = <FormArray>this.editProgramForm.controls['mashsteps'];
	control.removeAt(n)

	// and fix the order fields
	const msctrl = <FormArray>this.editProgramForm.controls['mashsteps'];

	let i = 0;
	for ( let step of msctrl.controls ) {
	    step.get('order').setValue(i);
	    i = i + 1;
	}
    }

    initHop() {
	const boiltime = this.editProgramForm.controls['boiltime'];
	return this._fb.group({
	    attime: [0, [Validators.required, IntValidator.minValue(0), IntValidator.maxValue(boiltime.value)]],
	    name: ['', [Validators.required, Validators.minLength(3)]],
	    quantity: [0, [Validators.required, IntValidator.minValue(1)]]
	});
    }

    initHops(hops: any) {
	let boiltime = 60;
	if ( this.editProgramForm ) {
	    boiltime = this.editProgramForm.controls['boiltime'].value;
	}
	let fbhops = [];
	for ( let hop of hops ) {
	    fbhops.push(this._fb.group({
		attime: [hop['attime'], [Validators.required, IntValidator.minValue(0), IntValidator.maxValue(boiltime)]],
		name: [hop['hopname'], [Validators.required, Validators.minLength(3)]],
		quantity: [hop['hopqty'], [Validators.required, IntValidator.minValue(1)]]
	    }));
	}
	return fbhops;
    }

    addHop() {
	const control = <FormArray>this.editProgramForm.controls['hops'];
	control.push(this.initHop());
    }

    removeHop(i: number) {
	const control = <FormArray>this.editProgramForm.controls['hops'];
	control.removeAt(i)
    }

    save(model: FormGroup) {
	/*
	console.log('save ', model);
	console.log('save ', model.value);
	*/
	this.api.saveProgram(model.value).subscribe(
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
