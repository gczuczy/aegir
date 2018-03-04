import { Component, OnInit } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import 'rxjs/add/operator/switchMap';
import { Validators, FormGroup, FormArray, FormBuilder,
	 ValidatorFn, AbstractControl } from '@angular/forms';

import { IntValidator } from '../int-validator';
import { Program } from './program';
import { ApiService } from '../api.service';

@Component({
  templateUrl: './load-program.component.html',
  styleUrls: ['./load-program.component.css']
})
export class LoadProgramComponent implements OnInit {
    public program: Program;
    public loadProgramForm: FormGroup;
    public mindate;
    public maxdate;
    public errors = [];

    constructor(private _fb: FormBuilder,
		private api: ApiService,
		private route: ActivatedRoute,
		private router: Router) {
	let now = new Date();
	this.mindate = now.toISOString().slice(0,16);
	now = new Date(now.getTime() + 3600*168*1000);
	this.maxdate = now.toISOString().slice(0,16);
    }

    ngOnInit() {
	// TODO: setting the initial date is still buggy
	this.route.params
	    .switchMap((params: Params) => this.api.getProgram(params['id']))
	    .subscribe(prog => {
		this.program = prog['data'];
		console.log('Program to load: ', this.program);
		this.loadProgramForm = this._fb.group({
		    id: [this.program['id']],
		    startmode: ["now"],
		    startat: [(new Date()).toISOString().slice(0,16), [Validators.required]],
		    volume: [35, [Validators.required, IntValidator.minValue(5), IntValidator.maxValue(80)]],
		});
	    });
    }

    submit(model: FormGroup) {
	console.log(model);
	this.api.loadProgram(model.value).subscribe(
	    resp => {
		this.errors = null;
		this.router.navigate(['brew']);
	    },
	    err => {
		let errobj = err.json();
		//console.log('Error obj: ', errobj);
		this.errors = errobj['errors'];
	    }
	);
    }
}
