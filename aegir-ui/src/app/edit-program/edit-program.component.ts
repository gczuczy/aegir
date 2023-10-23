import { Component, OnInit } from '@angular/core';

import { Validators, FormGroup, FormArray, FormControl,
	 FormBuilder, AbstractControl, ValidatorFn, ValidationErrors } from '@angular/forms';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { switchMap } from 'rxjs/operators';

import { ApiService } from '../api.service';
import { apiProgram, apiProgramHopStep, apiProgramMashStep,
	 apiSaveProgramData } from '../api.types';

import { initMashStep, initHop, defineValidators,
	 programForm, toApiProgram } from '../programs.common';

@Component({
  selector: 'app-edit-program',
  templateUrl: './edit-program.component.html',
  styleUrls: ['./edit-program.component.css']
})
export class EditProgramComponent implements OnInit, programForm {

  errors: string[] | null = null;

  public minmashtemp:number = 37;
  public maxmashtemp:number = 85;

  public hasmash:boolean = true;
  public hasboil:boolean = true;

  private program: apiProgram | null = null;

  editProgramForm: FormGroup = new FormGroup({});

  constructor(private api: ApiService,
	      private fb: FormBuilder,
	      private route: ActivatedRoute,
	      private router: Router) {
  }

  ngOnInit() {
    this.route.params
      .pipe(
	switchMap( (params:Params) => this.api.getProgram(params['id']) )
      ).subscribe(
	(data:apiProgram) => {
	  this.program = data;
	  this.hasmash = !data.nomash;
	  this.hasboil = !data.noboil;

	  this.editProgramForm = new FormGroup({
	    id: new FormControl(this.program.id),
	    name: new FormControl(this.program.name,
				  [Validators.required,
				   Validators.minLength(3)]),
	    starttemp: new FormControl(this.program.starttemp,
				       [Validators.required,
					(control: AbstractControl) => Validators.min(25)(control),
					(control: AbstractControl) => Validators.max(85)(control)]),
	    endtemp: new FormControl(this.program.endtemp,
				     [Validators.required,
				      (control: AbstractControl) => Validators.min(37)(control),
				      (control: AbstractControl) => Validators.max(90)(control)]),
	    boiltime: new FormControl(this.program.boiltime,
				      [Validators.required,
				       (control: AbstractControl) => Validators.min(30)(control),
				       (control: AbstractControl) => Validators.max(300)(control)]),
	    hasmash: new FormControl(this.hasmash, []),
	    mashsteps: this.fb.array(this.initMashSteps(this.program.mashsteps, this.program.endtemp)),
	    hasboil: new FormControl(this.hasboil, []),
	    hops: this.fb.array(this.initHops(this.program.hops, this.program.boiltime))
	  });
	  defineValidators(this.editProgramForm, this);
	}
      );
  } // ngOnInit

  initMashSteps(steps: apiProgramMashStep[], endtemp: number) {
    let fbsteps = [];
    for ( let step of steps ) {
      fbsteps.push(this.fb.group({
	order: [step.orderno],
	temp: [step.temperature, [Validators.required,
				  (control: AbstractControl) => Validators.min(20)(control),
				  (control: AbstractControl) => Validators.max(endtemp)(control)]],
	holdtime: [step.holdtime, [Validators.required]]
      }));
    }
    return fbsteps;
  }

  initHops(hops: apiProgramHopStep[], boiltime: number) {
    let fbhops = [];
    for ( let hop of hops ) {
      fbhops.push(this.fb.group({
	attime: [hop.attime, [Validators.required,
			      (control: AbstractControl) => Validators.min(0)(control),
			      (control: AbstractControl) => Validators.max(boiltime)(control)]],
	name: [hop.hopname, [Validators.required, Validators.minLength(3)]],
	quantity: [hop.hopqty, [Validators.required,
				(control: AbstractControl) => Validators.min(1)(control)]],
      }));
    }
    return fbhops;
  }

  addMashStep() {
    const control = this.editProgramForm.get('mashsteps') as FormArray;
    let ms = initMashStep(control.length,
			  this.fb,
			  this.editProgramForm.get('endtemp')!.value);
    control.push(ms);
  }

  removeMashStep(n: number) {
    const control = this.editProgramForm.get('mashsteps') as FormArray;
    control.removeAt(n)

    // and fix the order fields
    const msctrl = this.editProgramForm.get('mashsteps') as FormArray;

    let i = 0;
    for ( let step of msctrl.controls ) {
      step.get('order')!.setValue(i);
      i = i + 1;
    }
  }

  addHop() {
    const control = this.editProgramForm.get('hops') as FormArray;
    control.push(initHop(this.editProgramForm,
			 this.fb));
  }

  removeHop(i: number) {
    const control = this.editProgramForm.get('hops') as FormArray;
    control.removeAt(i)
  }

  onHasmashChange(event:any) {
    this.hasmash = event as boolean;
  }

  onHasboilChange(event:any) {
    this.hasboil = event as boolean;
  }

  get mashSteps(): FormArray {
    return this.editProgramForm.get('mashsteps') as FormArray;
  }

  get hops(): FormArray {
    return this.editProgramForm.get('hops') as FormArray;
  }

  save(model: FormGroup) {
    /*
      console.log('save ', model);
      console.log('save ', model.value);
    */
    let p  = toApiProgram(model);
    this.api.saveProgram(p).subscribe(
      (resp:apiSaveProgramData) => {
	let progid = resp.progid;
	this.errors = null;
	this.api.announceUpdate();
	//console.log('Program Added with response: ', resp);
	this.router.navigate(['programs', progid, 'view']);
      },
      (err:any) => {
	let errobj = err.json();
	//console.log('Error obj: ', errobj);
	this.errors = errobj['errors'];
      }
    );
  }
}
