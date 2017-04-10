import { Component, OnInit } from '@angular/core';
import { Validators, FormGroup, FormArray, FormBuilder } from '@angular/forms';

import { Program, ProgramMashTemp } from './program';

@Component({
  templateUrl: './add-program.component.html',
  styleUrls: ['./add-program.component.css']
})
export class AddProgramComponent implements OnInit {
    public addProgramForm: FormGroup;

    constructor(private _fb: FormBuilder) {
	console.log("ctor");
    }

    ngOnInit() {
	console.log("ngOnInit()");
	this.addProgramForm = this._fb.group({
	    name: ['', [Validators.required, Validators.minLength(3)]],
	    endtemp: [80, Validators.required],
	    boiltime: [60, Validators.required],
	    mashsteps: this._fb.array([this.initMashStep(0)]),
	    hops: this._fb.array([])
	});
	console.log("ngOnInit(): ", this.addProgramForm);
    }

    initMashStep(n: number) {
	return this._fb.group({
	    order: [n],
	    temp: [42, Validators.required],
	    holdtime: [15, Validators.required]
	});
    }

    addMashStep() {
	const control = <FormArray>this.addProgramForm.controls['mashsteps'];
	let ms = this.initMashStep(control.length);
	control.push(ms);
    }

    removeMashStep(i: number) {
	const control = <FormArray>this.addProgramForm.controls['mashsteps'];
	control.removeAt(i)
    }

    initHop() {
	console.log("initHop");
	return this._fb.group({
	    attime: [0, Validators.required],
	    name: ['', Validators.required],
	    quantity: [0, Validators.required]
	});
    }

    addHop() {
	console.log('addHop1');
	const control = <FormArray>this.addProgramForm.controls['hops'];
	console.log('addHop2');
	control.push(this.initHop());
    }

    removeHop(i: number) {
	console.log('removeHop1');
	const control = <FormArray>this.addProgramForm.controls['hops'];
	console.log('removeHop2');
	control.removeAt(i)
    }

    save(model: Program) {
	console.log(model);
    }

}
