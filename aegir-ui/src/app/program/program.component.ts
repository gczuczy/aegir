import { Component } from '@angular/core';
import { ActivatedRoute, Params, Router } from '@angular/router';
import { switchMap } from 'rxjs/operators';

import { ApiService } from '../api.service';
import { apiProgram, apiProgramDeleteResponse } from '../api.types';

@Component({
  selector: 'app-program',
  templateUrl: './program.component.html',
  styleUrls: ['./program.component.css']
})
export class ProgramComponent {

  program: apiProgram|null = null;

  constructor(private api: ApiService,
	      private route: ActivatedRoute,
	      private router: Router) {
  }

  ngOnInit() {
    this.route.params
      .pipe(switchMap((params:Params) => this.api.getProgram(params['id'])))
      .subscribe(
	(prog:apiProgram) => {
	  this.program = prog;
	}
      );
  }

  onEdit() {
    this.router.navigate(['programs', this.program!.id, 'edit']);
  }

  onLoad() {
    this.router.navigate(['programs', this.program!.id, 'load']);
  }

  onDelete() {
    if ( confirm("Are you sure to delete this program?") ) {
      this.api.delProgram(this.program!.id!)
	.subscribe(
	  (resp:apiProgramDeleteResponse) => {
	    this.api.announceUpdate();
	    this.router.navigate(['programs']);
	    //console.log('Program Added with response: ', resp);
	  },
	  err => {
	    console.log('Error obj: ', err);
	  }
	);
      console.log("Delete ", this.program);
    }
  } // onDelete
}
