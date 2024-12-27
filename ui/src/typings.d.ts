import { AbstractControl } from '@angular/forms';

declare module '@angular/forms' {
  export interface AbstractControl {
    _old: any;
  }
}

