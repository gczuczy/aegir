import { Component, OnInit } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { switchMap } from 'rxjs/operators';

import { Validators, FormGroup, FormBuilder,
	 AbstractControl, FormControl } from '@angular/forms';

import { ApiService } from '../api.service';
import { apiProgram, apiBrewLoadProgramRequest,
	 LoadProgramStartMode } from '../api.types';

@Component({
  selector: 'app-load-program',
  templateUrl: './load-program.component.html',
  styleUrls: ['./load-program.component.css']
})
export class LoadProgramComponent {
  public program: apiProgram | null = null;
  public loadProgramForm: FormGroup = new FormGroup({});
  public mindate: string;
  public maxdate: string;
  public errors: string[] = [];

  constructor(private fb: FormBuilder,
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
      .pipe(switchMap((params: Params) => this.api.getProgram(params['id'])))
      .subscribe(
	(prog:apiProgram) => {
	  this.program = prog;
	  //console.log('Program to load: ', this.program);
	  this.loadProgramForm = new FormGroup({
	    id: new FormControl(this.program!.id!),
	    startmode: new FormControl("now"),
	    startat: new FormControl((new Date()).toISOString().slice(0,16),
				     [Validators.required]),
	    volume: new FormControl(35,
				    [Validators.required,
				     (control: AbstractControl) => Validators.min(5)(control),
				     (control: AbstractControl) => Validators.max(80)(control)])
	  });
	});
  }

  public submit(model: FormGroup) {
    let data: apiBrewLoadProgramRequest = {
      id: model.value!.id,
      startat: model.value.startat,
      startmode: LoadProgramStartMode.Now,
      volume: model.value!.volume,
    };
    if ( model.value.startmode == "timed" ) {
      data.startmode = LoadProgramStartMode.Timed;
    }
    this.api.loadProgram(data).subscribe(
      (resp:any) => {
	this.errors = [];
	this.router.navigate(['brew']);
      },
      (error:any) => {
	//console.log('load-p error', error);
	this.errors = [];
	this.errors.push(error['error']['message']);
      }
    );
  }

}
