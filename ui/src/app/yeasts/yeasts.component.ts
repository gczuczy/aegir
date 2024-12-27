import { Component, OnInit, OnDestroy } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { switchMap } from 'rxjs/operators';
import { Validators, FormGroup, FormArray, FormControl,
	 FormBuilder, AbstractControl, ValidatorFn,
	 ValidationErrors } from '@angular/forms';

import { ApiService } from '../api.service';
import { apiYeast } from '../api.types';

import { faPlus, faPenToSquare, faXmark } from '@fortawesome/free-solid-svg-icons';

interface yeast {
  data: apiYeast,
  editMode: boolean,
  editForm: FormGroup,
}

@Component({
  selector: 'app-yeasts',
  templateUrl: './yeasts.component.html',
  styleUrls: ['./yeasts.component.css']
})
export class YeastsComponent {
  public faPlus = faPlus;
  public faPenToSquare = faPenToSquare;
  public faXmark = faXmark;

  public addRow: boolean = false;
  private fermdid: number|null = null;
  public yeasts: yeast[] = [];
  public addYeastForm: FormGroup = new FormGroup({
    name: new FormControl("", [Validators.required,
			       Validators.minLength(3)]),
    attenuation: new FormControl(80, [Validators.required,
			   Validators.min(50),
			   Validators.max(100)]),
    abv: new FormControl(10, [Validators.required,
			   Validators.min(5),
			   Validators.max(30)]),
    mintemp: new FormControl(15, [Validators.required,
			   Validators.min(10),
			   Validators.max(30)]),
    maxtemp: new FormControl(18, [Validators.required,
			   Validators.min(10),
			   Validators.max(30)]),
  });

  constructor(private api: ApiService,
    private route: ActivatedRoute,
    private router: Router) {
  }

  ngOnInit() {
    this.route.parent!.params
      .subscribe(
	(params:Params) => {
	  this.fermdid = params['fermdid'];
	  this.updateYeasts();
	}
      );
  }

  updateYeasts() {
    this.api.getYeasts(this.fermdid!).subscribe(
      (data:apiYeast[]) => {
	//console.log(data);
	let ys: yeast[] = [];

	for (let y of data) {
	  ys.push({'data': y,
		   'editMode': false,
		   'editForm': this.createEditFormGroup(y)});
	}

	this.yeasts = ys.sort(
	  (a,b) => a.data.name.localeCompare(b.data.name));
      }
    );
  }

  createEditFormGroup(data: apiYeast): FormGroup {
    return new FormGroup({
      name: new FormControl(data.name, [Validators.required,
					Validators.minLength(3)]),
      attenuation: new FormControl(data.attenuation, [Validators.required,
						      Validators.min(50),
						      Validators.max(100)]),
      abv: new FormControl(data.abv, [Validators.required,
				      Validators.min(5),
				      Validators.max(30)]),
      mintemp: new FormControl(data.mintemp, [Validators.required,
					      Validators.min(10),
					      Validators.max(30)]),
      maxtemp: new FormControl(data.maxtemp, [Validators.required,
					      Validators.min(10),
					      Validators.max(30)]),
  });
  }

  add(model: FormGroup) {
    let data: apiYeast = {
      name: model.get('name')!.value,
      attenuation: model.get('attenuation')!.value,
      abv: model.get('abv')!.value,
      mintemp: model.get('mintemp')!.value,
      maxtemp: model.get('maxtemp')!.value,
    };
    this.api.addYeast(this.fermdid!, data).subscribe(
      (resp:apiYeast) => {
	this.updateYeasts();
	this.addRow = false;
      }
    );
  }

  edit(yeastid: number, model: FormGroup) {
    let data: apiYeast = {
      id: yeastid,
      name: model.get('name')!.value,
      attenuation: model.get('attenuation')!.value,
      abv: model.get('abv')!.value,
      mintemp: model.get('mintemp')!.value,
      maxtemp: model.get('maxtemp')!.value,
    };
    this.api.updateYeast(this.fermdid!, data).subscribe(
      (resp:apiYeast) => {
	this.updateYeasts();
	for (let y of this.yeasts) {
	  if ( y.data.id! == yeastid ) {
	    y.editMode = false;
	    break;
	  }
	}
      }
    );
  }

  del(yeastid: number) {
    //console.log("delete", yeastid);
    if (confirm("Kill the yeast?")) {
      this.api.deleteYeast(this.fermdid!, yeastid).subscribe(
	(data:any) => {
	  this.updateYeasts();
	}
      );
    }
  }
}
