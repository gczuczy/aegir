import { Component, OnInit } from '@angular/core';
import { Validators, FormGroup, FormArray, FormControl,
	 FormBuilder, AbstractControl, ValidatorFn, ValidationErrors } from '@angular/forms';
import { Router } from '@angular/router';

import { ApiService } from '../api.service';
import { apiAddProgramData,
	 apiProgram, apiProgramHopStep, apiProgramMashStep } from '../api.types';

import { initMashStep, initHop, defineValidators,
	 programForm, toApiProgram } from '../programs.common';

import { Enzyme, EnzymeList } from '../enzymes';

@Component({
  selector: 'app-add-program',
  templateUrl: './add-program.component.html',
  styleUrls: ['./add-program.component.css']
})
export class AddProgramComponent implements OnInit, programForm {

  public enzymes = JSON.parse(JSON.stringify(EnzymeList)) as Enzyme[];

  errors: string[] | null = null;

  public minmashtemp:number = 37;
  public maxmashtemp:number = 80;

  public hasmash:boolean = true;
  public hasboil:boolean = true;

  addProgramForm: FormGroup = new FormGroup({
    name: new FormControl("", [Validators.required,
			       Validators.minLength(3)]),
    starttemp: new FormControl(this.minmashtemp, [Validators.required,
						  (control: AbstractControl) => Validators.min(25)(control),
						  (control: AbstractControl) => Validators.max(85)(control)]),
    endtemp: new FormControl(this.maxmashtemp, [Validators.required,
						(control: AbstractControl) => Validators.min(37)(control),
						(control: AbstractControl) => Validators.max(90)(control)]),
    boiltime: new FormControl(60, [Validators.required,
				   (control: AbstractControl) => Validators.min(30)(control),
				   (control: AbstractControl) => Validators.max(300)(control)]),
    hasmash: new FormControl(this.hasmash, []),
    mashsteps: this.fb.array([initMashStep(0, this.fb, this.maxmashtemp)]),
    hasboil: new FormControl(this.hasboil, []),
    hops: this.fb.array([])
  });

  constructor(private api: ApiService,
	      private fb: FormBuilder,
	      private router: Router) {
  }

  ngOnInit() {
    this.errors = null;

    this.hasmash = true;
    this.hasboil = true;

    defineValidators(this.addProgramForm, this);
    this.addProgramForm.valueChanges.subscribe(
      (data:FormGroup) => {
	if ( this.hasmash ) this.recalcEnzymeDuration(data);
      }
    );
    if ( this.hasmash ) this.recalcEnzymeDuration(this.addProgramForm.value);
  } // ngOnInit

  recalcEnzymeDuration(data: any) {
    if ( !this.hasmash ) return;

    for ( let rest of this.enzymes ) {
      rest.duration = 0;
    }

    for ( let step of data.mashsteps!  ) {
      for ( let rest of this.enzymes ) {
	if ( rest.min <= step.temp && rest.max >= step.temp ) {
	  rest.duration! += step.holdtime as number;
	}
      }
    }
  }

  public addMashStep() {
    const control = this.addProgramForm.get('mashsteps')! as FormArray;
    let endtemp = this.addProgramForm.get('endtemp')!.value;
    let ms = initMashStep(control.length, this.fb, endtemp);
    control.push(ms);
  }

  public removeMashStep(n: number) {
    const control = <FormArray>this.addProgramForm.controls['mashsteps'];
    control.removeAt(n)

    // and fix the order fields
    const msctrl = <FormArray>this.addProgramForm.controls['mashsteps'];

    let i = 0;
    for ( let step of msctrl.controls ) {
      step.get('order')!.setValue(i);
      i = i + 1;
    }
  }

  public addHop() {
    const control = <FormArray>this.addProgramForm.controls['hops'];
    control.push(initHop(this.addProgramForm, this.fb));
  }

  public removeHop(i: number) {
    const control = <FormArray>this.addProgramForm.controls['hops'];
    control.removeAt(i)
  }

  onHasmashChange(event:any) {
    this.hasmash = event['checked'];
  }

  onHasboilChange(event:any) {
    this.hasboil = event['checked'];
  }

  get mashSteps(): FormArray {
    return this.addProgramForm.get('mashsteps') as FormArray;
  }

  get hops(): FormArray {
    return this.addProgramForm.get('hops') as FormArray;
  }

  public save(model: FormGroup) {
    //console.log('save ', model);
    /*
    let hasmash = model.get('hasmash')!.value;
    let hasboil = model.get('hasboil')!.value;
    let p : apiProgram = {
      starttemp: model.get('starttemp')!.value,
      endtemp: model.get('endtemp')!.value,
      boiltime: model.get('boiltime')!.value,
      name: model.get('name')!.value,
      nomash: !hasmash,
      noboil: !hasboil,
      mashsteps: this.getMashSteps(model.get('mashsteps')! as FormArray),
      hops: this.getHops(model.get('hops')! as FormArray)
      };
    */
    let p  = toApiProgram(model);
    this.api.addProgram(p).subscribe(
      (resp:apiAddProgramData) => {
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
